# 🌌 SYNX HOSTING — Self-Hosted Minecraft Platform

<div align="center">

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=for-the-badge&logo=cplusplus)
![Qt 6](https://img.shields.io/badge/Qt-6.8-green.svg?style=for-the-badge&logo=qt)
![React](https://img.shields.io/badge/React-18-61DAFB.svg?style=for-the-badge&logo=react)
![TailwindCSS](https://img.shields.io/badge/Tailwind-3.4-38B2AC.svg?style=for-the-badge&logo=tailwind-css)
![License](https://img.shields.io/badge/License-MIT-purple.svg?style=for-the-badge)

**A high-performance, independent, self-hosted Minecraft server orchestrator that turns your personal PC into a 24/7 dedicated game hosting node.**

[Features](#-key-features) • [Architecture](#-architecture) • [Getting Started](#-getting-started) • [Dashboard](#-customer-web-dashboard) • [Desktop Admin](#-desktop-admin-app) • [License](#-license)

</div>

---

## ⚡ Why SYNX HOSTING?

Most self-hosting solutions require heavy Linux virtual machines, Docker orchestration, or complex Pterodactyl setups with multiple dependencies. 

**SYNX HOSTING** is built from the ground up natively in **C++20** specifically for **Windows 11 / Windows Server**, using native **Windows Job Objects** for kernel-enforced hard RAM and CPU limits, paired with a modern React customer dashboard and a Qt 6 desktop management console.

---

## ✨ Key Features

- 🛡️ **Kernel Hardware Sandboxing**: Process isolation using native Windows Job Objects (`JOB_OBJECT_LIMIT_JOB_MEMORY`, `JOB_OBJECT_CPU_RATE_CONTROL`). Zero zombie processes.
- 🌐 **Modern Web Dashboard**: Glassmorphic, dark-purple web interface built with React 18, TypeScript, and Tailwind CSS.
- 🖥️ **Desktop Admin Console**: Native Windows 11 application built with C++20 and Qt 6.8.
- 🕹️ **Java + Bedrock Crossplay**: Pre-configured **GeyserMC & Floodgate** support on Bedrock UDP port `19132`.
- 🔓 **Cracked / Offline Support**: Native toggle for `online-mode=false` with automatic **SkinsRestorer** configuration for offline skins (TLauncher, Salwyrr, PojavLauncher).
- 🔄 **Multi-Version Protocol Support**: Integrated **ViaVersion**, **ViaBackwards**, and **ViaRewind** — clients from `1.8` to `1.21.x+` connect seamlessly.
- ⚡ **Real-Time Terminal Console**: WebSocket-driven live console stream with command execution (`/op`, `/gamemode`, `/say`).
- 💾 **Instant Verified Snapshots**: One-click `.tar.gz` and `.zip` instance backup engine with SHA-256 integrity checksum verification.
- 🐕 **Crash Watchdog**: Automated crash detection with rate-limited exponential backoff restarts.
- 🔌 **Dual IPC & REST Server**: Simultaneous Windows Named Pipe (`\\.\pipe\synx_agent_ipc`) and HTTP/1.1 REST + WebSocket server (`http://127.0.0.1:8443`).

---

## 🏗️ Architecture

```
               ┌────────────────────────────────────────────────────────┐
               │                 SYNX HOSTING PLATFORM                  │
               └────────────────────────────────────────────────────────┘
                                           │
         ┌─────────────────────────────────┴─────────────────────────────────┐
         ▼                                                                   ▼
┌───────────────────────┐                                         ┌───────────────────────┐
│ Customer Web Panel    │                                         │ SYNX Desktop Admin    │
│ React + TypeScript    │                                         │ C++20 + Qt 6.8.3      │
│ (Port 8443)           │                                         │ (Native Windows GUI)  │
└───────────┬───────────┘                                         └───────────┬───────────┘
            │ HTTP/WS                                                         │ Named Pipe / REST
            └──────────────────────────────┬──────────────────────────────────┘
                                           ▼
                 ┌───────────────────────────────────────────────────┐
                 │         SYNX HOST AGENT SERVICE (C++20)           │
                 │   - Instance Manager      - Console Manager       │
                 │   - Port & Network Pool   - Backup Manager        │
                 │   - Crash Watchdog        - Native WinSock2/Pipes │
                 └─────────────────────────┬─────────────────────────┘
                                           │
                         Windows Job Objects Kernel Isolation
                                           │
         ┌─────────────────────────────────┼─────────────────────────────────┐
         ▼                                 ▼                                 ▼
┌─────────────────┐               ┌─────────────────┐               ┌─────────────────┐
│ Minecraft 1     │               │ Minecraft 2     │               │ Bedrock / Geyser│
│ Paper / Purpur  │               │ Vanilla / Forge │               │ UDP 19132       │
│ Port 25565      │               │ Port 25566      │               │ Crossplay Relay │
└─────────────────┘               └─────────────────┘               └─────────────────┘
```

---

## 🚀 Getting Started

### Prerequisites

- **OS**: Windows 10 / 11 (64-bit)
- **Java**: OpenJDK 21 LTS or newer
- **Compiler**: MSVC 2022 (C++20)
- **Build Tools**: CMake 3.20+, Ninja
- **Qt** *(Optional for Desktop Admin)*: Qt 6.8+ MSVC 64-bit
- **Node.js**: v18+ (for Web Dashboard build)

### Quick Build & Run

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/SasynDev/synx-minecraft-hosting.git
   cd synx-minecraft-hosting
   ```

2. **Build Web Dashboard**:
   ```bash
   cd web
   pnpm install
   pnpm build
   cd ..
   ```

3. **Configure & Build C++ Engine**:
   ```powershell
   # Initialize MSVC environment
   cmake -B build -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.8.3\msvc2022_64"
   ninja -C build
   ```

4. **Launch Host Agent**:
   ```powershell
   .\build\src\agent\synx-agent.exe --console
   ```

5. **Open Web Dashboard**:
   Open [http://localhost:8443](http://localhost:8443) in your browser!

6. **Launch Desktop Admin GUI**:
   ```powershell
   .\build\src\admin\synx-admin.exe
   ```

---

## 🌐 Connecting Friends

| Connection Type | Address | Notes |
| :--- | :--- | :--- |
| **Direct (Host PC)** | `localhost:25565` | For playing on the host machine |
| **Same Wi-Fi / LAN** | `192.168.x.x:25565` | For friends connected to your home router |
| **Internet (WAN)** | `your-public-ip:25565` | Requires router port 25565 forwarded |
| **Custom Domain** | `play.yourdomain.com:25565` | Configurable directly in web dashboard |
| **Bedrock Mobile** | `192.168.x.x:19132` | Supported out of the box via GeyserMC |

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
Created with ❤️ by **SasynDev**.

