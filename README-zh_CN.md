# BotMasterXL - A LLM Driven in-game bot (PoC)

<p align="center">
<img src="./assets/witch.png" width="70" height="70">
</p>

*此项目为个人技术展示作品，展现从底层网络到高层AI集成的开发能力。*

[前端仓库: bot-master-xl-web](https://github.com/81Vm3/bot-master-xl-web)

BotMasterXL是一个基于大语言模型(LLM)的高级Bot矩阵框架 (Proof of concept)，为开发者提供创建完全自主机器人的可能

项目结合C++17、网络编程、集成ColAndreas和人工智能技术，为SAMP游戏环境提供AI。同时支持并发操作，允许同时多开多个bot

⚠️ 请注意，大模型可能产生意外的决策结果。使用本项目即表示您同意，自行承担因此产生的任何后果，作者不承担任何责任。


![Alt text](./assets/1.png)

## 快速开始

1. 下载本仓库Release的最新版本
2. 配置 `data/prompt.md` 提示词，一般情况下你需要针对具体的服务器来修改
3. 启动BotMasterXL，这会在7070端口上开启API服务器
4. 打开浏览器 localhost:3000，查看管理后端

## 技术栈
- libhv -> 后端API服务器
- sqlite -> 数据库
- ColAndreas -> 碰撞检测，寻路
- RakNet -> 网络库，用于与SAMP服务器通信
- spdlog -> 日志管理

## 决策系统
- **函数调用(Function Calling)架构**: 可集成多种LLM提供商，支持OpenAI
- **自感知能力**: 可以感知周围环境并作出智能决策
- **会话管理**: 具有一套自主开发的上下文管理系统
- **异步处理**: 非阻塞的LLM响应处理机制，可以同时开启多个LLM Bot

## 函数调用以及已实现的Tool

#### **SelfStatusTools (自身状态工具)**
- `get_position` - 获取机器人当前位置坐标
- `get_password` - 获取服务器密码
- `get_self_status` - 获取完整的机器人状态信息
- `get_chatbox_history` - 获取未读聊天消息

#### **SituationAwarenessTools (环境感知工具)**
- `list_vehicles` - 列出300m内所有载具
- `list_players` - 列出300m内所有玩家
- `list_objects` - 列出300m内所有物体(最多100个)
- `list_objects_text` - 列出300m内带文字的物体
- `list_pickups` - 列出300m内所有拾取物
- `list_labels` - 列出300m内所有3D文字标签
- `list_server_player` - 列出服务器内所有玩家

#### **WorldInteractionTools (世界交互工具)**
- `goto` - 移动到指定坐标(使用路径寻找)
- `forced_goto` - 强制移动到指定坐标(忽略碰撞)
- `random_explore` - 随机探索附近位置
- `chat` - 发送聊天消息
- `command` - 发送游戏命令
- `dialog_response` - 响应对话框
- `send_pickup` - 拾取指定物品

## 待完成

- [ ] 添加长期记忆系统
- [ ] 添加更多tool
- [ ] 添加游戏载具支持