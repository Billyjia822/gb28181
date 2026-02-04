# GB28181 设备端工程

基于GB/T 28181-2016标准的设备端协议栈实现，支持视频监控设备的注册、心跳、目录查询、设备信息查询、设备状态查询、语音广播、语音对讲、历史视音频下载、历史视音频回放等功能。

## 项目结构

```
Gb28181/
├── 3rd/                    # 第三方库
│   ├── osip/              # osip协议栈
│   │   ├── include/
│   │   └── src/
│   └── exosip/            # exosip协议栈
│       ├── include/
│       └── src/
├── include/               # 头文件
│   ├── sip/              # SIP协议模块
│   ├── rtp/              # RTP传输模块
│   ├── media/            # 媒体处理模块
│   ├── device/           # 设备管理模块
│   ├── ps/               # PS流封装模块
│   └── utils/            # 工具类
├── src/                  # 源文件
│   ├── sip/
│   ├── rtp/
│   ├── media/
│   ├── device/
│   ├── ps/
│   └── main.cpp
├── config/               # 配置文件
│   └── device_config.json
├── examples/             # 示例代码
├── tests/                # 测试代码
├── CMakeLists.txt        # CMake构建配置
└── README.md             # 项目说明
```

## 核心模块

### 1. SIP协议模块 (src/sip/)
- `sip_manager.h/cpp` - SIP管理器，处理注册、心跳等SIP信令
- `sip_message.h/cpp` - SIP消息封装和解析
- `sip_transport.h/cpp` - SIP传输层，基于UDP

### 2. 设备管理模块 (src/device/)
- `device_manager.h/cpp` - 设备信息管理，通道管理
- 支持设备目录查询响应
- 支持设备信息查询响应
- 支持设备状态查询响应

### 3. RTP传输模块 (src/rtp/)
- `rtp_manager.h/cpp` - RTP数据包收发
- 支持H.264/H.265视频流传输
- 支持G.711音频流传输
- 支持PS流传输

### 4. PS流封装模块 (src/ps/)
- `ps_muxer.h/cpp` - MPEG-2 PS流封装
- 支持H.264/H.265视频编码
- 支持G.711/AAC音频编码

### 5. 工具类 (include/utils/)
- `logger.h` - 日志系统
- `config_loader.h` - 配置文件加载
- `thread_pool.h` - 线程池

## 编译说明

### Windows (使用CMake)

```bash
# 创建构建目录
mkdir build
cd build

# 生成Visual Studio项目
cmake ..

# 编译
cmake --build . --config Release

# 运行
cd bin
./gb28181_device.exe
```

### Linux

```bash
# 创建构建目录
mkdir build
cd build

# 编译
cmake ..
make

# 运行
cd bin
./gb28181_device
```

### 指定本地IP和服务器IP

```bash
./gb28181_device <local_ip> <server_ip>
```

## 配置说明

配置文件位于 `config/device_config.json`，主要配置项：

```json
{
  "device": {
    "deviceId": "34020000001320000001",
    "deviceName": "GB28181 Camera",
    "manufacturer": "GB28181 Inc.",
    "model": "IPC-1000"
  },
  "sip": {
    "localIp": "192.168.1.100",
    "localPort": 5060,
    "serverIp": "192.168.1.1",
    "serverPort": 5060,
    "username": "34020000001320000001",
    "password": "12345678"
  },
  "rtp": {
    "localIp": "192.168.1.100",
    "basePort": 50000
  }
}
```

## 功能特性

- [x] SIP注册/注销
- [x] 心跳保活
- [x] 设备目录查询
- [x] 设备信息查询
- [x] 设备状态查询
- [x] PS流封装
- [x] RTP传输
- [x] H.264视频编码
- [x] G.711音频编码

## 第三方依赖

- **osip2** - SIP协议栈
- **libeXosip2** - eXtended osip，提供更高级的SIP功能

## 注意事项

1. 本项目中的osip和exosip为简化实现，生产环境建议使用完整的官方库
2. 设备ID编码遵循GB/T 28181-2016标准
3. 默认使用UDP传输SIP信令
4. PS流封装遵循ISO/IEC 13818-1标准

## 许可证

本项目仅供学习和研究使用。

## 参考资料

- GB/T 28181-2016 《公共安全视频监控联网系统信息传输、交换、控制技术要求》
- RFC 3261 - SIP: Session Initiation Protocol
- RFC 3550 - RTP: A Transport Protocol for Real-time Applications
- ISO/IEC 13818-1 - MPEG-2 Systems
# gb28181
