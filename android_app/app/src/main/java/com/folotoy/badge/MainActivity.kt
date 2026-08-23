package com.folotoy.badge

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.pm.PackageManager
import android.content.res.ColorStateList
import android.graphics.Typeface
import android.os.Build
import android.os.Bundle
import android.view.Gravity
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

/**
 * FoloToy Badge 安卓端:手动扫描连接名牌,写入/读取 姓名/顶部文字/职位/状态。
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
        private const val DEVICE_NAME = "FoloToy-Badge"
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

    // GATT 读写必须串行:一次一个,等回调完成再下一个。
    private val readQueue = mutableListOf<BluetoothGattCharacteristic>()
    private val writeQueue = mutableListOf<Pair<BluetoothGattCharacteristic, String>>()

    // ---- UI ----
    private lateinit var statusChip: Chip
    private lateinit var nameEt: EditText
    private lateinit var topEt: EditText
    private lateinit var titleEt: EditText
    private lateinit var statusEt: EditText
    private lateinit var scanBtn: MaterialButton
    private lateinit var readBtn: MaterialButton
    private lateinit var writeBtn: MaterialButton

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

        formCard.addView(form)
        root.addView(formCard)

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
        val wanted = listOf(CHR_NAME, CHR_TOP, CHR_TITLE, CHR_STATUS)
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
        )
        for ((uuid, value) in fields) {
            svc.getCharacteristic(uuid)?.let { writeQueue.add(it to value) }
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
        g.writeCharacteristic(item.first, item.second.toByteArray(Charsets.UTF_8),
            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
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
}