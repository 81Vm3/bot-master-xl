# BotMasterXL - A LLM Driven in-game bot (PoC)

*此项目为个人技术展示作品，展现从底层网络到高层AI集成的开发能力。*

## 项目简介

BotMasterXL是一个基于大语言模型(LLM)的高级Bot矩阵框架，为开发者提供创建完全自主智能机器人的可能

项目结合C++17、网络编程、集成ColAndreas和人工智能技术，为SAMP游戏环境提供AI

## LLM驱动的决策系统
- **函数调用(Function Calling)架构**: 可集成多种LLM提供商，支持OpenAI
- **自感知能力**: 机器人能够感知周围环境并作出智能决策
- **会话管理**: 完整的对话历史记录和上下文维护
- **异步处理**: 非阻塞的LLM响应处理机制

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

#### **其他工具模块**
- **BotLLMTools** - 工具注册中心,整合所有工具模块
- **MemoryTools** - 记忆管理工具(待实现)
- **ToolHelpers** - 工具辅助函数库


### 架构设计
- **模块化设计**: 基于现代C++17的RAII资源管理
- **单例模式**: 中央应用管理器(CApp)统一协调各个子系统
- **微服务架构**: RESTful API服务器支持热插拔服务
- **多线程支持**: 线程安全的资源池和并发处理

### 工具与功能
- **函数调度器**: 动态函数注册和调用系统
- **工具集成**: 丰富的机器人操作工具库

