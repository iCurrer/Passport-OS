package com.folotoy.badge

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.widget.*
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.ContextCompat
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
    private lateinit var statusTv: TextView
    private lateinit var nameEt: EditText
    private lateinit var topEt: EditText
    private lateinit var titleEt: EditText
    private lateinit var statusEt: EditText

    // 请求权限结果
    private val permLauncher = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { result ->
        if (result.values.all { it }) {
            statusTv.text = "权限已授予,点击「扫描」"
        } else {
            statusTv.text = "缺少权限,无法使用蓝牙"
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildUi()
        ensurePermissions()
    }

    // ---------- UI(代码构建,避免额外布局文件) ----------
    private fun buildUi() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 48, 32, 32)
        }

        fun label(text: String) = TextView(this).apply {
            this.text = text
            textSize = 18f
            setPadding(0, 24, 0, 4)
        }

        fun field(text: String) = EditText(this).apply {
            setText(text)
            textSize = 18f
            hint = text
            maxLines = 1
        }

        statusTv = TextView(this).apply {
            text = "未连接"
            textSize = 15f
            setTextColor(ContextCompat.getColor(this@MainActivity, android.R.color.holo_blue_dark))
        }

        root.addView(
            Button(this).apply {
                text = "扫描并连接"
                setOnClickListener { startScan() }
            }
        )
        root.addView(statusTv)

        root.addView(label("姓名"))
        nameEt = field("")
        root.addView(nameEt)

        root.addView(label("顶部文字"))
        topEt = field("")
        root.addView(topEt)

        root.addView(label("职位"))
        titleEt = field("")
        root.addView(titleEt)

        root.addView(label("状态"))
        statusEt = field("")
        root.addView(statusEt)

        root.addView(
            Button(this).apply {
                text = "读取当前值"
                setOnClickListener { readAll() }
            }
        )
        root.addView(
            Button(this).apply {
                text = "写入名牌"
                setOnClickListener { writeAll() }
            }
        )

        // 外层滚动容器
        val scroll = ScrollView(this).apply { addView(root) }
        setContentView(
            LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                gravity = Gravity.TOP
                addView(scroll, ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT))
            }
        )
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
            statusTv.text = "权限已授予,点击「扫描并连接」"
        } else {
            permLauncher.launch(needed.toTypedArray())
        }
    }

    // ---------- 扫描 ----------
    @SuppressLint("MissingPermission")
    private fun startScan() {
        val adapter = bluetoothAdapter ?: run { statusTv.text = "设备不支持蓝牙"; return }
        if (!adapter.isEnabled) { statusTv.text = "请先打开手机蓝牙"; return }
        stopScan()

        statusTv.text = "扫描中...(需名牌处于广播状态)"
        scanCallback = object : ScanCallback() {
            override fun onScanResult(callbackType: Int, result: ScanResult) {
                val name = result.device.name
                if (name == DEVICE_NAME) {
                    targetDevice = result.device
                    statusTv.text = "找到 $DEVICE_NAME,连接中..."
                    connect(result.device)
                }
            }
        }
        adapter.bluetoothLeScanner.startScan(scanCallback)
        // 12 秒后没找到就停止
        statusTv.postDelayed({ stopScan() }, 12000)
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
                    runOnUiThread { statusTv.text = "已连接,发现服务..." }
                    g.discoverServices()
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    runOnUiThread { statusTv.text = "连接断开" }
                }
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            if (status != BluetoothGatt.GATT_SUCCESS) {
                runOnUiThread { statusTv.text = "服务发现失败" }
                return
            }
            runOnUiThread { statusTv.text = "已连接,可读取/写入" }
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
            runOnUiThread {
                statusTv.text = "写入 ${chr.uuid} ${if (status == BluetoothGatt.GATT_SUCCESS) "成功" else "失败"}"
            }
            writeNext()  // 写完一个,继续下一个
        }
    }

    // ---------- 读 / 写(串行) ----------
    @SuppressLint("MissingPermission")
    private fun readAll() {
        val g = gatt ?: return
        val svc = g.getService(SVC) ?: run { statusTv.text = "未找到服务"; return }
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
        val g = gatt ?: run { statusTv.text = "未连接"; return }
        val svc = g.getService(SVC) ?: run { statusTv.text = "未找到服务"; return }
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
        val item = writeQueue.removeFirstOrNull() ?: run { statusTv.text = "写入完成"; return }
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
