# VNC 增强功能说明 / VNC Enhancement Guide

## 改进内容 / Improvements

本次更新为 noVNC 添加了以下功能：

This update adds the following features to noVNC:

### 1. 剪贴板支持 / Clipboard Support

- ✅ 双向剪贴板同步（容器 ↔ 浏览器）
- ✅ 自动检测并同步剪贴板内容
- ✅ 支持文本复制粘贴操作
- ✅ Bidirectional clipboard sync (Container ↔ Browser)
- ✅ Automatic clipboard detection and sync
- ✅ Text copy/paste operations supported

### 2. 移动设备键盘支持 / Mobile Keyboard Support

- ✅ 浮动键盘按钮（右下角 ⌨️ 图标）
- ✅ 支持中文输入法
- ✅ 支持特殊按键（回车、退格、Tab、方向键等）
- ✅ 自动检测移动设备并启用优化
- ✅ Floating keyboard button (⌨️ icon at bottom-right)
- ✅ Chinese/IME input method support
- ✅ Special keys support (Enter, Backspace, Tab, Arrow keys, etc.)
- ✅ Auto-detect mobile devices and enable optimizations

### 3. 触摸手势优化 / Touch Gesture Optimization

- ✅ 缩放模式自动调整
- ✅ 触摸友好的界面
- ✅ Auto-scaling mode adjustment
- ✅ Touch-friendly interface

## 使用方法 / Usage

### 桌面浏览器 / Desktop Browser

1. **访问 noVNC**: http://localhost:8083
2. **剪贴板操作**:
   - 从容器复制：在 VNC 界面中选择文本，会自动复制到系统剪贴板
   - 粘贴到容器：使用 Ctrl+V (Windows/Linux) 或 Cmd+V (Mac) 粘贴到 VNC 界面
3. **Clipboard Operations**:
   - Copy from container: Select text in VNC, it auto-copies to system clipboard
   - Paste to container: Use Ctrl+V (Windows/Linux) or Cmd+V (Mac) in VNC interface

### 移动设备 / Mobile Devices

1. **访问 noVNC**: http://your-server-ip:8083
2. **唤出键盘 / Show Keyboard**:
   - 点击右下角的 ⌨️ 浮动按钮
   - Click the ⌨️ floating button at bottom-right
3. **输入文本 / Text Input**:
   - 键盘弹出后，直接输入文字（支持中文输入法）
   - After keyboard appears, type directly (Chinese IME supported)
   - 输入会实时传送到 VNC 会话
   - Input is sent to VNC session in real-time
4. **特殊按键 / Special Keys**:
   - 回车、退格、Tab、方向键等会被正确识别
   - Enter, Backspace, Tab, Arrow keys are properly recognized

### 剪贴板使用技巧 / Clipboard Tips

#### 从容器复制 / Copy from Container
```
1. 在 VNC 界面中选择要复制的文本
2. 文本自动复制到浏览器剪贴板
3. 可以粘贴到其他应用

1. Select text in VNC interface
2. Text auto-copies to browser clipboard
3. Paste to other applications
```

#### 粘贴到容器 / Paste to Container
```
1. 在本地复制文本 (Ctrl+C / Cmd+C)
2. 在 VNC 界面中点击目标位置
3. 粘贴 (Ctrl+V / Cmd+V)

1. Copy text locally (Ctrl+C / Cmd+C)
2. Click target location in VNC
3. Paste (Ctrl+V / Cmd+V)
```

## 技术细节 / Technical Details

### 更新内容 / Changes Made

1. **Dockerfile 改进**:
   - 升级到最新版 noVNC
   - 安装 Python3 和 websockify
   - 添加自定义配置脚本

2. **supervisord.conf 更新**:
   - 使用新的 websockify 命令
   - 优化 VNC 代理配置

3. **novnc-config.js**:
   - 自动检测移动设备
   - 实现剪贴板事件监听
   - 创建浮动键盘按钮
   - 处理特殊按键映射

4. **docker-compose.yml**:
   - 添加环境变量支持

### 端口说明 / Port Information

- `8083`: noVNC Web 访问端口 / noVNC web access port
- `8998`: VNC 内部端口 / Internal VNC port

## 故障排除 / Troubleshooting

### 剪贴板不工作 / Clipboard Not Working

1. 确保浏览器允许剪贴板访问权限
2. 使用 HTTPS 连接（某些浏览器要求）
3. 检查浏览器控制台是否有错误信息

1. Ensure browser allows clipboard access
2. Use HTTPS connection (required by some browsers)
3. Check browser console for error messages

### 移动键盘不显示 / Mobile Keyboard Not Showing

1. 确认是否在移动设备上访问
2. 检查右下角是否有 ⌨️ 按钮
3. 尝试刷新页面

1. Confirm accessing from mobile device
2. Check if ⌨️ button appears at bottom-right
3. Try refreshing the page

### 中文输入无法使用 / Chinese Input Not Working

1. 点击 ⌨️ 按钮唤出键盘
2. 在键盘上切换到中文输入法
3. 输入的中文会逐字发送到 VNC

1. Click ⌨️ button to show keyboard
2. Switch to Chinese IME on keyboard
3. Chinese characters sent to VNC character by character

## 重新构建 / Rebuild

更新配置后需要重新构建镜像：
After updating configuration, rebuild the image:

```bash
# 重新构建镜像 / Rebuild image
docker-compose build

# 重启服务 / Restart service
docker-compose down
docker-compose up -d

# 或者一步完成 / Or do it in one step
docker-compose up -d --build
```

## 浏览器兼容性 / Browser Compatibility

| 浏览器 / Browser | 剪贴板 / Clipboard | 移动键盘 / Mobile Keyboard |
|-----------------|-------------------|---------------------------|
| Chrome/Edge     | ✅                | ✅                        |
| Firefox         | ✅                | ✅                        |
| Safari          | ⚠️ (需HTTPS)      | ✅                        |
| Mobile Chrome   | ✅                | ✅                        |
| Mobile Safari   | ⚠️ (需HTTPS)      | ✅                        |

⚠️ Safari 需要 HTTPS 连接才能完全支持剪贴板功能
⚠️ Safari requires HTTPS for full clipboard support

## 安全建议 / Security Recommendations

1. 建议使用 HTTPS 访问（配置反向代理）
2. 限制 8083 端口只允许信任的 IP 访问
3. 定期更新 Docker 镜像

1. Recommend using HTTPS (configure reverse proxy)
2. Restrict port 8083 to trusted IPs only
3. Regularly update Docker images

## 高级配置 / Advanced Configuration

如需自定义键盘按钮位置或样式，编辑 `docker/novnc-config.js`:

To customize keyboard button position or style, edit `docker/novnc-config.js`:

```javascript
// 修改按钮位置 / Change button position
keyboardBtn.style.cssText = `
    position: fixed;
    bottom: 20px;  // 距离底部距离 / Distance from bottom
    right: 20px;   // 距离右侧距离 / Distance from right
    ...
`;
```

## 贡献 / Contributing

欢迎提交 Issue 和 Pull Request！
Issues and Pull Requests are welcome!
