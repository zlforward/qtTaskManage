# Qt任务管理系统

这是一个使用Qt框架开发的任务管理系统，支持拖放操作的看板（Kanban）风格的任务管理工具。

## 功能特点

- 看板式布局，直观展示任务状态（待办、进行中、已完成）
- 支持任务拖放操作，轻松更改任务状态
- 每个任务卡片显示标题、描述、优先级、状态和截止日期
- 使用SQLite数据库持久化保存任务数据
- 支持添加、编辑和删除任务
- 美观的界面设计和动画效果

## 技术栈

- C++
- Qt框架
- Qt图形视图框架（Graphics View Framework）
- SQLite数据库

## 编译与运行

1. 确保已安装Qt开发环境
2. 打开QtConsoleApplication1.sln文件
3. 构建解决方案
4. 运行应用程序

## 截图

（此处可添加应用程序截图）

## 实现细节

- 使用QGraphicsView和QGraphicsScene实现任务看板界面
- TaskCard类继承自QGraphicsWidget，实现了任务卡片的展示和交互功能
- 使用Qt的信号与槽机制实现拖放操作和状态更新
- 通过SQLite数据库实现任务数据的持久化存储 