package com.folotoy.badge

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.content.pm.PackageManager
import android.content.res.ColorStateList
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Typeface
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.*
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
import com.google.android.material.button.MaterialButton
import com.google.android.material.card.MaterialCardView
import com.google.android.material.textfield.TextInputLayout
import com.google.android.material.chip.Chip
import java.util.UUID
import java.util.zip.CRC32

/**
 * FoloToy Badge 安卓端:手动扫描连接名牌,写入/读取 姓名/顶部文字/职位/状态/简介/网站/GitHub。
 * 提供 240×320 Passport 实时预览,编辑字段即时刷新。
 * App 退出(onDestroy)时停止扫描并关闭 GATT / 释放蓝牙,后台不占用。
 */
class MainActivity : ComponentActivity() {

    companion object {
        // 与固件 main/ble.c 一致
        private val SVC = UUID.fromString("0000FFE0-0000-1000-8000-00805F9B34FB")
        private val CHR_NAME = UUID.fromString("0000FFE1-0000-1000-8000-00805F9B34FB")
        private val CHR_TOP = UUID.fromString("0000FFE2-0000-1000-8000-00805F9B34FB")
        private val CHR_TITLE = UUID.fromString("0000FFE3-0000-1000-8000-00805F9B34FB")
        private val CHR_STATUS = UUID.fromString("0000FFE4-0000-1000-8000-00805F9B34FB")
        private val CHR_BIO = UUID.fromString("0000FFE5-0000-1000-8000-00805F9B34FB")
        private val CHR_WEBSITE = UUID.fromString("0000FFE6-0000-1000-8000-00805F9B34FB")
        private val CHR_GITHUB = UUID.fromString("0000FFE7-0000-1000-8000-00805F9B34FB")
        private val CHR_AV_CTRL = UUID.fromString("0000FFE8-0000-1000-8000-00805F9B34FB")
        private val CHR_AV_DATA = UUID.fromString("0000FFE9-0000-1000-8000-00805F9B34FB")
        private const val DEVICE_NAME = "FoloToy-Badge"
        private const val AV_W = 80
        private const val AV_H = 80
        private const val AV_SIZE = AV_W * AV_H * 2   // 12,800 字节 RGB565
        private const val AV_CHUNK = 244               // BLE 分块(对齐固件 AV_DATA)
    }

    private val bluetoothManager: BluetoothManager by lazy {
        getSystemService(BluetoothManager::class.java)
    }
    private val bluetoothAdapter: BluetoothAdapter? by lazy {
        bluetoothManager.adapter
    }
    private var gatt: BluetoothGatt? = null
    private var scanCallback: ScanCallback? = null
    private var targetDevice: BluetoothDevice? = null

    // GATT 读写必须串行:一次一个,等回调完成再下一个。值用字节数组(支持原始二进制)。
    private val readQueue = mutableListOf<BluetoothGattCharacteristic>()
    private val writeQueue = mutableListOf<Pair<BluetoothGattCharacteristic, ByteArray>>()

    // ---- UI ----
    private lateinit var statusChip: Chip
    private lateinit var nameEt: EditText
    private lateinit var topEt: EditText
    private lateinit var titleEt: EditText
    private lateinit var statusEt: EditText
    private lateinit var bioEt: EditText
    private lateinit var websiteEt: EditText
    private lateinit var githubEt: EditText
    private lateinit var scanBtn: MaterialButton
    private lateinit var readBtn: MaterialButton
    private lateinit var writeBtn: MaterialButton
    private lateinit var passportPreview: PassportPreviewView

    // 图库选头像
    private val pickAvatarLauncher =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            if (uri != null) handleAvatarUri(uri)
        }

    // 请求权限结果
    private val permLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { result ->
        if (result.values.all { it }) {
            setConnectionState(ConnectionState.DISCONNECTED, "点击「扫描并连接」")
        } else {
            setConnectionState(ConnectionState.ERROR, "缺少权限,无法使用蓝牙")
        }
    }

    // ---- 连接状态枚举 ----
    private enum class ConnectionState {
        DISCONNECTED, SCANNING, CONNECTING, CONNECTED, ERROR
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildUi()
        ensurePermissions()
    }

    // ---------- UI(使用 Material3 组件构建) ----------
    private fun buildUi() {
        // 根容器:浅灰背景 + 垂直滚动
        val scrollView = ScrollView(this).apply {
            setBackgroundColor(themeColor(com.google.android.material.R.attr.colorSurface))
            setClipToPadding(false)
            // 顶部/左右留白,避免内容顶到状态栏或贴边
            setPadding(dp(20), dp(24), dp(20), dp(24))
        }

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(0, 0, 0, dp(12))
        }

        // ---- 顶部标题区域 ----
        val headerCard = MaterialCardView(this).apply {
            setCardBackgroundColor(ContextCompat.getColor(context, android.R.color.transparent))
            cardElevation = 0f
            strokeWidth = 0
            radius = 0f
            setContentPadding(dp(4), dp(8), dp(4), dp(0))
        }
        val headerInner = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }
        headerInner.addView(TextView(this).apply {
            text = "AI 名牌"
            textSize = 26f
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(themeColor(com.google.android.material.R.attr.colorOnSurface))
        })
        headerInner.addView(TextView(this).apply {
            text = "AI 电子名牌配置工具"
            textSize = 14f
            setTextColor(themeColor(com.google.android.material.R.attr.colorOnSurfaceVariant))
            setPadding(0, dp(2), 0, 0)
        })
        headerCard.addView(headerInner)
        root.addView(headerCard)

        // ---- 连接状态区域 ----
        val statusCard = MaterialCardView(this).apply {
            radius = dp(16).toFloat()
            cardElevation = dp(2).toFloat()
            strokeWidth = 0
            setContentPadding(dp(16), dp(14), dp(16), dp(14))
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
        }

        val statusRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }

        statusChip = Chip(this).apply {
            text = "未连接"
            chipIcon = null
            setChipBackgroundColorResource(com.google.android.material.R.color.m3_sys_color_dynamic_dark_on_surface)
            setTextColor(ContextCompat.getColor(this@MainActivity, android.R.color.white))
            textSize = 13f
            chipMinHeight = dp(32).toFloat()
            isClickable = false
            isCheckable = false
        }

        scanBtn = MaterialButton(this).apply {
            text = "扫描"
            textSize = 14f
            setIconResource(android.R.drawable.ic_menu_search)
            iconSize = dp(18)
            iconGravity = MaterialButton.ICON_GRAVITY_TEXT_START
            setOnClickListener { startScan() }
            strokeWidth = 0
            cornerRadius = dp(20)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, dp(40)
            ).apply { marginStart = dp(12) }
        }

        statusRow.addView(statusChip)
        statusRow.addView(scanBtn)
        statusCard.addView(statusRow)
        root.addView(statusCard)

        // ---- 输入表单卡片 ----
        val formCard = MaterialCardView(this).apply {
            radius = dp(20).toFloat()
            cardElevation = dp(2).toFloat()
            strokeWidth = 0
            setContentPadding(dp(16), dp(16), dp(16), dp(8))
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
        }
        val form = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }

        fun field(label: String, default: String = ""): EditText {
            val til = TextInputLayout(this@MainActivity).apply {
                // 用浮动标签作字段名,避免与框内文字重叠
                this.hint = label
                boxBackgroundMode = TextInputLayout.BOX_BACKGROUND_OUTLINE
                setBoxCornerRadii(dp(12).toFloat(), dp(12).toFloat(), dp(12).toFloat(), dp(12).toFloat())
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { topMargin = dp(8) }
            }
            val et = EditText(this@MainActivity).apply {
                setText(default)
                textSize = 16f
                maxLines = 1
            }
            til.addView(et)
            form.addView(til)
            return et
        }

        nameEt = field("姓名")
        topEt = field("顶部文字")
        titleEt = field("职位")
        statusEt = field("状态")
        bioEt = field("简介")
        websiteEt = field("网站")
        githubEt = field("GitHub")

        formCard.addView(form)
        root.addView(formCard)

        // ---- 240×320 Passport Preview(实时预览) ----
        val previewCard = MaterialCardView(this).apply {
            radius = dp(20).toFloat()
            cardElevation = dp(2).toFloat()
            strokeWidth = 0
            setContentPadding(dp(16), dp(16), dp(16), dp(16))
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
        }
        val previewColumn = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }
        previewColumn.addView(TextView(this).apply {
            text = "Passport 预览"
            textSize = 15f
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(themeColor(com.google.android.material.R.attr.colorOnSurface))
        })
        passportPreview = PassportPreviewView(this).apply {
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(8) }
        }
        previewColumn.addView(passportPreview)
        previewColumn.addView(MaterialButton(this).apply {
            text = "选择图片并上传头像"
            textSize = 14f
            setOnClickListener { pickAvatarLauncher.launch("image/*") }
            strokeWidth = 0
            cornerRadius = dp(20)
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(44)
            ).apply { topMargin = dp(8) }
        })
        previewCard.addView(previewColumn)
        root.addView(previewCard)

        // 编辑任一字段即实时刷新预览
        val watcher = object : android.text.TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, a: Int, b: Int, c: Int) {}
            override fun onTextChanged(s: CharSequence?, a: Int, b: Int, c: Int) {}
            override fun afterTextChanged(s: android.text.Editable?) {
                passportPreview.invalidate()
            }
        }
        listOf(nameEt, topEt, titleEt, statusEt, bioEt, websiteEt, githubEt)
            .forEach { it.addTextChangedListener(watcher) }

        // ---- 操作按钮卡片 ----
        val actionCard = MaterialCardView(this).apply {
            radius = dp(20).toFloat()
            cardElevation = dp(2).toFloat()
            strokeWidth = 0
            setContentPadding(dp(16), dp(16), dp(16), dp(16))
            layoutParams = LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(16) }
        }
        val actionInner = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
        }

        readBtn = MaterialButton(this).apply {
            text = "读取"
            textSize = 14f
            setIconResource(android.R.drawable.ic_menu_upload)
            iconSize = dp(18)
            iconGravity = MaterialButton.ICON_GRAVITY_TEXT_START
            setOnClickListener { readAll() }
            strokeWidth = dp(1)
            strokeColor = ColorStateList.valueOf(themeColor(com.google.android.material.R.attr.colorPrimary))
            cornerRadius = dp(20)
            layoutParams = LinearLayout.LayoutParams(
                0, dp(44), 1f
            )
        }

        writeBtn = MaterialButton(this).apply {
            text = "写入名牌"
            textSize = 14f
            setIconResource(android.R.drawable.ic_menu_send)
            iconSize = dp(18)
            iconGravity = MaterialButton.ICON_GRAVITY_TEXT_START
            setOnClickListener { writeAll() }
            strokeWidth = 0
            cornerRadius = dp(20)
            layoutParams = LinearLayout.LayoutParams(
                0, dp(44), 1f
            ).apply { marginStart = dp(12) }
        }

        actionInner.addView(readBtn)
        actionInner.addView(writeBtn)
        actionCard.addView(actionInner)
        root.addView(actionCard)

        // ---- 底部提示 ----
        root.addView(TextView(this).apply {
            text = "确保名牌处于广播状态，连接后即可读取/写入"
            textSize = 12f
            setTextColor(themeColor(com.google.android.material.R.attr.colorOnSurfaceVariant))
            gravity = Gravity.CENTER
            setPadding(0, dp(16), 0, 0)
        })

        scrollView.addView(root)
        setContentView(scrollView)
    }

    // ---------- 更新连接状态 UI ----------
    private fun setConnectionState(state: ConnectionState, message: String) {
        runOnUiThread {
            val chipColor = when (state) {
                ConnectionState.SCANNING -> android.R.color.holo_blue_dark
                ConnectionState.CONNECTING -> android.R.color.holo_orange_dark
                ConnectionState.CONNECTED -> android.R.color.holo_green_dark
                ConnectionState.ERROR -> android.R.color.holo_red_dark
                ConnectionState.DISCONNECTED -> com.google.android.material.R.color.m3_sys_color_dynamic_dark_on_surface
            }
            statusChip.text = message
            statusChip.setChipBackgroundColorResource(chipColor)
            statusChip.setTextColor(
                ContextCompat.getColor(
                    this,
                    if (state == ConnectionState.CONNECTED || state == ConnectionState.DISCONNECTED)
                        android.R.color.white
                    else
                        android.R.color.white
                )
            )
        }
    }

    // ---------- dp 转换辅助 ----------
    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density + 0.5f).toInt()

    // 从当前主题解析一个颜色属性(attr)到实际颜色值。
    // ⚠ ContextCompat.getColor() 只能接收 color 资源 ID,不能直接传 R.attr.*(会崩)。
    private fun themeColor(attrRes: Int): Int {
        val tv = android.util.TypedValue()
        return if (theme.resolveAttribute(attrRes, tv, true)) {
            if (tv.type == android.util.TypedValue.TYPE_REFERENCE) ContextCompat.getColor(this, tv.resourceId)
            else tv.data
        } else {
            0xFF808080.toInt()
        }
    }

    // ---------- 权限 ----------
    private fun ensurePermissions() {
        val needed = mutableListOf(Manifest.permission.ACCESS_FINE_LOCATION)
        if (Build.VERSION.SDK_INT >= 31) {
            needed.add(Manifest.permission.BLUETOOTH_SCAN)
            needed.add(Manifest.permission.BLUETOOTH_CONNECT)
        }
        val missing = needed.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        if (missing.isEmpty()) {
            setConnectionState(ConnectionState.DISCONNECTED, "点击「扫描」连接名牌")
        } else {
            permLauncher.launch(needed.toTypedArray())
        }
    }

    // ---------- 扫描 ----------
    @SuppressLint("MissingPermission")
    private fun startScan() {
        val adapter = bluetoothAdapter ?: run { setConnectionState(ConnectionState.ERROR, "设备不支持蓝牙"); return }
        if (!adapter.isEnabled) { setConnectionState(ConnectionState.ERROR, "请先打开手机蓝牙"); return }
        stopScan()

        setConnectionState(ConnectionState.SCANNING, "扫描中...")
        scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                val name = result.device.name
                if (name == DEVICE_NAME) {
                    targetDevice = result.device
                    setConnectionState(ConnectionState.CONNECTING, "找到 $DEVICE_NAME,连接中...")
                    connect(result.device)
                }
            }
        }
        adapter.bluetoothLeScanner.startScan(scanCallback)
        // 12 秒后没找到就停止
        statusChip.postDelayed({
            if (gatt == null) {
                stopScan()
                setConnectionState(ConnectionState.DISCONNECTED, "未找到设备,点击重试")
            }
        }, 12000)
    }

    @SuppressLint("MissingPermission")
    private fun stopScan() {
        val cb = scanCallback ?: return
        try { bluetoothAdapter?.bluetoothLeScanner?.stopScan(cb) } catch (_: Exception) {}
        scanCallback = null
    }

    @SuppressLint("MissingPermission")
    private fun connect(device: BluetoothDevice) {
        stopScan()
        gatt?.close()
        gatt = device.connectGatt(this, false, gattCallback)
    }

    // ---------- GATT 回调 ----------
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    setConnectionState(ConnectionState.CONNECTING, "已连接,发现服务...")
                    g.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    setConnectionState(ConnectionState.DISCONNECTED, "连接断开,点击重试")
                }
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                setConnectionState(ConnectionState.ERROR, "服务发现失败")
                return
            }
            setConnectionState(ConnectionState.CONNECTED, "已连接")
            readAll()
        }

        override fun onCharacteristicRead(g: BluetoothGatt, chr: BluetoothGattCharacteristic, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                val s = String(chr.value ?: ByteArray(0), Charsets.UTF_8)
                runOnUiThread {
                    when (chr.uuid) {
                        CHR_NAME -> nameEt.setText(s)
                        CHR_TOP -> topEt.setText(s)
                        CHR_TITLE -> titleEt.setText(s)
                        CHR_STATUS -> statusEt.setText(s)
                        CHR_BIO -> bioEt.setText(s)
                        CHR_WEBSITE -> websiteEt.setText(s)
                        CHR_GITHUB -> githubEt.setText(s)
                    }
                }
            }
            readNext()   // 读完一个,继续下一个(无论成功失败)
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, chr: BluetoothGattCharacteristic, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                setConnectionState(ConnectionState.CONNECTED, "写入成功")
            } else {
                setConnectionState(ConnectionState.ERROR, "写入失败")
            }
            writeNext()  // 写完一个,继续下一个
        }
    }

    // ---------- 读 / 写(串行) ----------
    @SuppressLint("MissingPermission")
    private fun readAll() {
        val g = gatt ?: return
        val svc = g.getService(SVC) ?: run { setConnectionState(ConnectionState.ERROR, "未找到服务"); return }
        val wanted = listOf(CHR_NAME, CHR_TOP, CHR_TITLE, CHR_STATUS,
            CHR_BIO, CHR_WEBSITE, CHR_GITHUB)
        readQueue.clear()
        readQueue.addAll(svc.characteristics.filter { it.uuid in wanted })
        readNext()
    }

    @SuppressLint("MissingPermission")
    private fun readNext() {
        val g = gatt ?: return
        val chr = readQueue.removeFirstOrNull() ?: return
        g.readCharacteristic(chr)
    }

    @SuppressLint("MissingPermission")
    private fun writeAll() {
        val g = gatt ?: run {
            setConnectionState(ConnectionState.ERROR, "未连接"); return
        }
        val svc = g.getService(SVC) ?: run {
            setConnectionState(ConnectionState.ERROR, "未找到服务"); return
        }
        writeQueue.clear()
        val fields = mapOf(
            CHR_NAME to nameEt.text.toString(),
            CHR_TOP to topEt.text.toString(),
            CHR_TITLE to titleEt.text.toString(),
            CHR_STATUS to statusEt.text.toString(),
            CHR_BIO to bioEt.text.toString(),
            CHR_WEBSITE to websiteEt.text.toString(),
            CHR_GITHUB to githubEt.text.toString(),
        )
        for ((uuid, value) in fields) {
            svc.getCharacteristic(uuid)?.let { writeQueue.add(it to value.toByteArray(Charsets.UTF_8)) }
        }
        writeNext()
    }

    @SuppressLint("MissingPermission")
    private fun writeNext() {
        val g = gatt ?: return
        val item = writeQueue.removeFirstOrNull() ?: run {
            setConnectionState(ConnectionState.CONNECTED, "写入完成")
            return
        }
        g.writeCharacteristic(item.first, item.second,
            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    }

    // ---------- 头像处理 + 上传 ----------
    private fun handleAvatarUri(uri: android.net.Uri) {
        val bmp = try {
            contentResolver.openInputStream(uri)?.use { BitmapFactory.decodeStream(it) }
        } catch (_: Exception) { null }
        if (bmp == null) { setConnectionState(ConnectionState.ERROR, "图片解码失败"); return }
        try {
            val rgb = toRgb565Le(centerCropSquare(bmp))
            bmp.recycle()
            uploadAvatar(rgb)
        } catch (e: Exception) {
            setConnectionState(ConnectionState.ERROR, "头像处理失败")
        }
    }

    // 居中裁剪成正方形
    private fun centerCropSquare(bmp: Bitmap): Bitmap {
        val s = minOf(bmp.width, bmp.height)
        val x = (bmp.width - s) / 2
        val y = (bmp.height - s) / 2
        return Bitmap.createBitmap(bmp, x, y, s, s)
    }

    // 缩放 + 转 80x80 RGB565(小端字节序,与固件 LV_COLOR_FORMAT_RGB565 一致)
    private fun toRgb565Le(bmp: Bitmap): ByteArray {
        val scaled = Bitmap.createScaledBitmap(bmp, AV_W, AV_H, true)
        val px = IntArray(AV_W * AV_H)
        scaled.getPixels(px, 0, AV_W, 0, 0, AV_W, AV_H)
        scaled.recycle()
        val out = ByteArray(AV_SIZE)
        var o = 0
        for (v in px) {
            val r = (v shr 16) and 0xFF
            val g = (v shr 8) and 0xFF
            val b = v and 0xFF
            val c = ((r shr 3) shl 11) or ((g shr 2) shl 5) or (b shr 3)
            out[o++] = (c and 0xFF).toByte()          // 低字节
            out[o++] = ((c shr 8) and 0xFF).toByte()  // 高字节
        }
        return out
    }

    // 通过 BLE 上传:写控制 "START <size> <crc>",再分块写数据(与固件 TASK-12 协议一致)
    @SuppressLint("MissingPermission")
    private fun uploadAvatar(bytes: ByteArray) {
        val g = gatt ?: run { setConnectionState(ConnectionState.ERROR, "未连接"); return }
        val svc = g.getService(SVC) ?: run { setConnectionState(ConnectionState.ERROR, "未找到服务"); return }
        val ctrl = svc.getCharacteristic(CHR_AV_CTRL) ?: run { setConnectionState(ConnectionState.ERROR, "无头像服务"); return }
        val data = svc.getCharacteristic(CHR_AV_DATA) ?: run { setConnectionState(ConnectionState.ERROR, "无头像服务"); return }
        if (bytes.size != AV_SIZE) { setConnectionState(ConnectionState.ERROR, "头像尺寸错误"); return }

        val crc = CRC32().also { it.update(bytes) }.value
        writeQueue.clear()
        writeQueue.add(ctrl to "START $AV_SIZE $crc".toByteArray(Charsets.UTF_8))
        for (i in bytes.indices step AV_CHUNK) {
            writeQueue.add(data to bytes.copyOfRange(i, minOf(i + AV_CHUNK, bytes.size)))
        }
        setConnectionState(ConnectionState.CONNECTED, "上传头像...")
        writeNext()
    }

    // ---------- App 退出:关闭蓝牙 ----------
    @SuppressLint("MissingPermission")
    override fun onDestroy() {
        stopScan()
        try { gatt?.disconnect() } catch (_: Exception) {}
        try { gatt?.close() } catch (_: Exception) {}
        gatt = null
        super.onDestroy()
    }

    // ---------- 240×320 Passport 预览视图(按 4:3 比例缩放) ----------
    inner class PassportPreviewView(context: Context) : View(context) {
        private val paint = android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG)
        private val cBg = 0xFF000000.toInt()
        private val cWhite = 0xFFFFFFFF.toInt()
        private val cGray = 0xFF8A8A8A.toInt()
        private val cAccent = 0xFF4CD964.toInt()
        private val cLine = 0xFF1F1F1F.toInt()
        private val cAvatar = 0xFF2A2A2A.toInt()

        // 保持 240:320(4:3)比例
        override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
            val w = MeasureSpec.getSize(widthMeasureSpec)
            setMeasuredDimension(w, (w * 4f / 3f).toInt())
        }

        override fun onDraw(canvas: android.graphics.Canvas) {
            val w = width.toFloat()
            val s = w / 240f   // 240px 参考宽度缩放

            canvas.drawColor(cBg)

            // 顶部文字(左上)
            drawText(canvas, 16 * s, 26 * s, topEt.text.toString().ifEmpty { "FoloToy" },
                cWhite, 14f, false, android.graphics.Paint.Align.LEFT)
            // 电量(右上,占位静态)
            drawText(canvas, 224 * s, 26 * s, "86%", cGray, 12f, false,
                android.graphics.Paint.Align.RIGHT)

            // 分隔线
            paint.color = cLine
            canvas.drawRect(0f, 34 * s, w, 34 * s + 2f * s, paint)

            // 头像占位(灰圆角方块,居中上段)
            val avW = 56f * s
            val avH = 110f * s
            val avX = (w - avW) / 2f
            val avY = 56f * s
            paint.color = cAvatar
            canvas.drawRoundRect(avX, avY, avX + avW, avY + avH, 8f * s, 8f * s, paint)

            // 姓名 / 职位 / 状态(居中)
            drawText(canvas, w / 2f, 208f * s, nameEt.text.toString().ifEmpty { "姓名" },
                cWhite, 24f, true, android.graphics.Paint.Align.CENTER)
            drawText(canvas, w / 2f, 232f * s, titleEt.text.toString(), cGray, 14f, false,
                android.graphics.Paint.Align.CENTER)
            drawText(canvas, w / 2f, 256f * s, statusEt.text.toString(), cAccent, 14f, false,
                android.graphics.Paint.Align.CENTER)

            // 底部 8 点 Page Indicator(第 1 点强调实心)
            val dot = 6f * s
            val gap = 12f * s
            val total = 7 * gap + dot
            val x0 = (w - total) / 2f
            for (i in 0 until 8) {
                val x = x0 + i * gap
                paint.color = if (i == 0) cAccent else cGray
                canvas.drawCircle(x + dot / 2f, 300f * s, dot / 2f, paint)
            }
        }

        private fun drawText(canvas: android.graphics.Canvas, x: Float, y: Float, text: String,
                             color: Int, px: Float, bold: Boolean, align: android.graphics.Paint.Align) {
            paint.color = color
            paint.textSize = px * (width.toFloat() / 240f)
            paint.typeface = if (bold) android.graphics.Typeface.DEFAULT_BOLD
                             else android.graphics.Typeface.DEFAULT
            paint.textAlign = align
            canvas.drawText(text, x, y, paint)
        }
    }
}