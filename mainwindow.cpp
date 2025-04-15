#include "mainwindow.h"
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QDateTime>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFormLayout>
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>
#include "reportdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      todoColumn(nullptr),
      inProgressColumn(nullptr),
      doneColumn(nullptr),
      m_taskDialog(nullptr),
      m_titleEdit(nullptr),
      m_descEdit(nullptr),
      m_priorityCombo(nullptr),
      m_deadlineEdit(nullptr),
      m_statusCombo(nullptr),
      m_currentEditCard(nullptr),
      m_scene(nullptr)
{
    ui->setupUi(this);
    setMinimumSize(1200, 800);
    setWindowTitle(QString::fromLocal8Bit("任务管理系统"));
    
    // 创建场景和视图
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);
    ui->graphicsView->setSceneRect(0, 0, 1150, 700);
    
    // 设置深蓝色背景（与图片中的风格一致）
    QLinearGradient gradient(0, 0, 0, ui->graphicsView->height());
    gradient.setColorAt(0, QColor(10, 35, 80));
    gradient.setColorAt(1, QColor(5, 15, 40));
    m_scene->setBackgroundBrush(gradient);
    
    // 启用拖动和缩放功能
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    // 创建状态列
    setupColumns();
    
    // 创建任务对话框
    setupTaskDialog();
    
    // 连接按钮信号与槽
    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteButtonClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::onClearButtonClicked);
    connect(ui->reportButton, &QPushButton::clicked, this, &MainWindow::onReportButtonClicked);
    
    // 初始化数据库并加载任务
    initDatabase();
    loadTasks();
    setupScene();
}

MainWindow::~MainWindow()
{
    saveTasks();
    
    // 清理资源
    if (m_taskDialog) {
        delete m_taskDialog;
        m_taskDialog = nullptr;
    }
    
    delete ui;
}

void MainWindow::setupColumns()
{
    // 列的宽度和高度
    const int columnWidth = 350;
    const int columnHeight = 650;
    const int columnSpacing = 30;
    
    // 创建列标题文本
    QGraphicsTextItem *todoTitle = m_scene->addText(QString::fromLocal8Bit("待办任务"), QFont(QString::fromLocal8Bit("微软雅黑"), 16, QFont::Bold));
    todoTitle->setDefaultTextColor(Qt::white);
    todoTitle->setPos(40, 30);
    
    QGraphicsTextItem *inProgressTitle = m_scene->addText(QString::fromLocal8Bit("进行中"), QFont(QString::fromLocal8Bit("微软雅黑"), 16, QFont::Bold));
    inProgressTitle->setDefaultTextColor(Qt::white);
    inProgressTitle->setPos(40 + columnWidth + columnSpacing, 30);
    
    QGraphicsTextItem *doneTitle = m_scene->addText(QString::fromLocal8Bit("已完成"), QFont(QString::fromLocal8Bit("微软雅黑"), 16, QFont::Bold));
    doneTitle->setDefaultTextColor(Qt::white);
    doneTitle->setPos(40 + (columnWidth + columnSpacing) * 2, 30);
    
    // 创建三个区域用于放置不同状态的任务
    QPen columnPen(QColor(255, 255, 255, 30));
    QBrush columnBrush(QColor(255, 255, 255, 15));
    
    todoColumn = m_scene->addRect(40, 70, columnWidth, columnHeight, columnPen, columnBrush);
    inProgressColumn = m_scene->addRect(40 + columnWidth + columnSpacing, 70, columnWidth, columnHeight, columnPen, columnBrush);
    doneColumn = m_scene->addRect(40 + (columnWidth + columnSpacing) * 2, 70, columnWidth, columnHeight, columnPen, columnBrush);
}

void MainWindow::setupTaskDialog()
{
    // 确保 m_taskDialog 已正确创建
    if (m_taskDialog) {
        delete m_taskDialog;
        m_taskDialog = nullptr;
    }
    
    // 使用QDialog而不是QWidget，QDialog对模态行为处理更好
    m_taskDialog = new QDialog(this);
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("创建新任务"));
    m_taskDialog->setMinimumWidth(400);
    
    // 设置为应用程序模态，这样用户必须先处理这个对话框
    m_taskDialog->setModal(true);
    
    // 设置对话框在显示时始终处于窗口的中央
    m_taskDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::MSWindowsFixedSizeDialogHint);
    
    // 避免对话框关闭时被删除
    m_taskDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    
    // 设置对话框位置居中
    QSize dialogSize(400, 300);
    m_taskDialog->resize(dialogSize);
    
    QVBoxLayout *layout = new QVBoxLayout(m_taskDialog);
    QFormLayout *formLayout = new QFormLayout();
    
    // 任务标题
    m_titleEdit = new QLineEdit(m_taskDialog);
    formLayout->addRow(QString::fromLocal8Bit("任务标题:"), m_titleEdit);
    
    // 任务描述
    m_descEdit = new QTextEdit(m_taskDialog);
    m_descEdit->setMaximumHeight(100);
    formLayout->addRow(QString::fromLocal8Bit("任务描述:"), m_descEdit);
    
    // 任务优先级
    m_priorityCombo = new QComboBox(m_taskDialog);
    m_priorityCombo->addItem(QString::fromLocal8Bit("低"), TaskCard::Low);
    m_priorityCombo->addItem(QString::fromLocal8Bit("中"), TaskCard::Medium);
    m_priorityCombo->addItem(QString::fromLocal8Bit("高"), TaskCard::High);
    m_priorityCombo->setCurrentIndex(1); // 默认为中优先级
    formLayout->addRow(QString::fromLocal8Bit("优先级:"), m_priorityCombo);
    
    // 任务状态
    m_statusCombo = new QComboBox(m_taskDialog);
    m_statusCombo->addItem(QString::fromLocal8Bit("待办"), TaskCard::Todo);
    m_statusCombo->addItem(QString::fromLocal8Bit("进行中"), TaskCard::InProgress);
    m_statusCombo->addItem(QString::fromLocal8Bit("已完成"), TaskCard::Done);
    m_statusCombo->setCurrentIndex(0); // 默认为待办状态
    formLayout->addRow(QString::fromLocal8Bit("状态:"), m_statusCombo);
    
    // 截止日期
    m_deadlineEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7), m_taskDialog);
    m_deadlineEdit->setCalendarPopup(true);
    formLayout->addRow(QString::fromLocal8Bit("截止日期:"), m_deadlineEdit);
    
    layout->addLayout(formLayout);
    
    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *cancelButton = new QPushButton(QString::fromLocal8Bit("取消"), m_taskDialog);
    QPushButton *okButton = new QPushButton(QString::fromLocal8Bit("确定"), m_taskDialog);
    
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    layout->addLayout(buttonLayout);
    
    // 连接信号和槽
    connect(cancelButton, &QPushButton::clicked, m_taskDialog, &QDialog::reject);
    connect(okButton, &QPushButton::clicked, this, &MainWindow::onTaskDialogAccepted);
    connect(m_taskDialog, &QDialog::rejected, [this]() {
        // 对话框被拒绝（取消）时刷新场景
        m_currentEditCard = nullptr; // 重置当前编辑卡片
        m_scene->update();
        ui->graphicsView->viewport()->update();
    });
}

void MainWindow::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("tasks.db");
    
    if (!m_db.open()) {
        qDebug() << "Error: " << m_db.lastError();
        return;
    }
    qDebug() << "Database opened successfully";
    
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS tasks ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "title TEXT, "
              "description TEXT, "
              "status INTEGER, "
              "priority INTEGER, "
              "deadline TEXT)");
}

void MainWindow::saveTasks()
{
    QSqlQuery query;
    query.exec("DELETE FROM tasks");
    
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            query.prepare("INSERT INTO tasks (title, description, status, priority, deadline) "
                         "VALUES (:title, :description, :status, :priority, :deadline)");
            query.bindValue(":title", card->title());
            query.bindValue(":description", card->description());
            query.bindValue(":status", card->status());
            query.bindValue(":priority", card->priority());
            
            if (card->deadline().isValid()) {
                query.bindValue(":deadline", card->deadline().toString(Qt::ISODate));
            } else {
                query.bindValue(":deadline", QVariant());
            }
            
            query.exec();
        }
    }
}

void MainWindow::loadTasks()
{
    qDebug() << "Loading tasks from database";
    
    // 移除场景中所有的任务卡片
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (dynamic_cast<TaskCard*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_cards.clear(); // 清空内部列表
    
    QSqlQuery query("SELECT title, description, status, priority, deadline FROM tasks");
    
    if (!query.exec()) {
        qDebug() << "Query error: " << query.lastError();
        return;
    }
    
    int count = 0;
    while (query.next()) {
        QString title = query.value(0).toString();
        QString description = query.value(1).toString();
        TaskCard::Status status = static_cast<TaskCard::Status>(query.value(2).toInt());
        TaskCard::Priority priority = static_cast<TaskCard::Priority>(query.value(3).toInt());
        
        QDateTime deadline;
        if (!query.value(4).isNull()) {
            deadline = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        }
        
        TaskCard *card = new TaskCard(title, description, priority, status, deadline);
        // 连接信号与槽
        connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        
        m_scene->addItem(card);
        m_cards.append(card); // 添加到内部列表
        count++;
    }
    
    qDebug() << "Loaded " << count << " tasks";
    
    // 如果没有任务，添加一些示例任务
    if (count == 0) {
        // 添加示例待办任务
        TaskCard *todo1 = new TaskCard(QString::fromLocal8Bit("完成项目计划"), 
                                      QString::fromLocal8Bit("制定详细的项目实施计划，包括时间表和资源分配"), 
                                      TaskCard::Medium, TaskCard::Todo, QDateTime::currentDateTime().addDays(2));
        connect(todo1, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(todo1, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(todo1);
        m_cards.append(todo1);
        
        TaskCard *todo2 = new TaskCard(QString::fromLocal8Bit("学习Qt编程"), 
                                      QString::fromLocal8Bit("完成Qt GUI编程的基础学习，掌握信号与槽机制"), 
                                      TaskCard::High, TaskCard::Todo, QDateTime::currentDateTime().addDays(7));
        connect(todo2, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(todo2, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(todo2);
        m_cards.append(todo2);
        
        // 添加示例进行中任务
        TaskCard *inProgress = new TaskCard(QString::fromLocal8Bit("开发任务管理系统"), 
                                           QString::fromLocal8Bit("实现基本的任务管理功能，包括创建、编辑和删除任务"), 
                                           TaskCard::High, TaskCard::InProgress, QDateTime::currentDateTime().addDays(5));
        connect(inProgress, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(inProgress, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(inProgress);
        m_cards.append(inProgress);
        
        // 添加示例已完成任务
        TaskCard *done = new TaskCard(QString::fromLocal8Bit("需求分析"), 
                                     QString::fromLocal8Bit("分析用户需求，确定系统功能和界面设计"), 
                                     TaskCard::Medium, TaskCard::Done, QDateTime::currentDateTime().addDays(-2));
        connect(done, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(done, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(done);
        m_cards.append(done);
    }

    // 在所有任务（加载的和示例的）都添加到场景后，调用 arrangeCards 进行布局
    arrangeCards();
}

void MainWindow::arrangeCards()
{
    // 在每一列中整理卡片位置
    const int cardSpacing = 20;
    int todoY = todoColumn->rect().y() + 20;
    int inProgressY = inProgressColumn->rect().y() + 20;
    int doneY = doneColumn->rect().y() + 20;
    
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            qreal x = 0;
            qreal y = 0;
            
            switch (card->status()) {
                case TaskCard::Todo:
                    x = todoColumn->rect().x() + 25;
                    y = todoY;
                    todoY += card->boundingRect().height() + cardSpacing;
                    break;
                case TaskCard::InProgress:
                    x = inProgressColumn->rect().x() + 25;
                    y = inProgressY;
                    inProgressY += card->boundingRect().height() + cardSpacing;
                    break;
                case TaskCard::Done:
                    x = doneColumn->rect().x() + 25;
                    y = doneY;
                    doneY += card->boundingRect().height() + cardSpacing;
                    break;
            }
            
            card->setPos(x, y);
        }
    }
}

TaskCard::Status MainWindow::getStatusFromPosition(qreal x)
{
    // 获取场景宽度并计算每列宽度
    qreal sceneWidth = m_scene->width();
    qreal columnWidth = sceneWidth / 3;
    
    // 根据x坐标确定状态
    if (x < columnWidth) {
        return TaskCard::Todo;
    } else if (x < 2 * columnWidth) {
        return TaskCard::InProgress;
    } else {
        return TaskCard::Done;
    }
}

void MainWindow::onAddButtonClicked()
{
    // 清除当前编辑卡片的引用
    m_currentEditCard = nullptr;
    
    // 设置对话框标题为添加任务
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("添加任务"));
    
    // 重置表单
    m_titleEdit->clear();
    m_descEdit->clear();
    m_priorityCombo->setCurrentIndex(0); // 低优先级
    m_statusCombo->setCurrentIndex(0);   // 待办状态
    m_deadlineEdit->setDateTime(QDateTime::currentDateTime().addDays(7));
    
    // 居中显示对话框
    m_taskDialog->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            m_taskDialog->size(),
            QApplication::desktop()->availableGeometry()
        )
    );
    
    // 执行对话框（模态显示）
    m_taskDialog->exec();
}

void MainWindow::onTaskDialogAccepted()
{
    QString title = m_titleEdit->text().trimmed();
    QString description = m_descEdit->toPlainText().trimmed();
    
    if (title.isEmpty()) {
        QMessageBox::warning(this, QString::fromLocal8Bit("警告"), 
                           QString::fromLocal8Bit("任务名称不能为空！"));
        return;
    }
    
    QDateTime deadline = m_deadlineEdit->dateTime();
    
    TaskCard::Priority priority;
    switch (m_priorityCombo->currentIndex()) {
        case 0: priority = TaskCard::Low; break;
        case 1: priority = TaskCard::Medium; break;
        case 2: priority = TaskCard::High; break;
        default: priority = TaskCard::Medium;
    }
    
    TaskCard::Status status;
    switch (m_statusCombo->currentIndex()) {
        case 0: status = TaskCard::Todo; break;
        case 1: status = TaskCard::InProgress; break;
        case 2: status = TaskCard::Done; break;
        default: status = TaskCard::Todo;
    }
    
    // 如果是编辑已有卡片
    if (m_currentEditCard) {
        m_currentEditCard->setTitle(title);
        m_currentEditCard->setDescription(description);
        m_currentEditCard->setPriority(priority);
        m_currentEditCard->setStatus(status);
        m_currentEditCard->setDeadline(deadline);
        
        // 清除当前编辑卡片的引用
        m_currentEditCard = nullptr;
        
        // 重新排列卡片
        arrangeCards();
    } 
    // 如果是添加新卡片
    else {
        createNewTask(title, description, priority, status, deadline);
    }
    
    // 保存任务到数据库
    saveTasks();
    
    // 接受对话框，对话框会自动关闭
    m_taskDialog->accept();
    
    // 刷新场景显示
    m_scene->update();
}

void MainWindow::createNewTask(const QString &title, const QString &description, 
                             TaskCard::Priority priority, TaskCard::Status status, 
                             const QDateTime &deadline)
{
    TaskCard *card = new TaskCard(title, description, priority, status, deadline);
    
    // 连接卡片的信号与槽
    connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
    connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
    
    // 设置卡片的初始位置(根据status确定列)
    m_scene->addItem(card);
    m_cards.append(card);
    
    // 重新排列卡片
    arrangeCards();
}

void MainWindow::onDeleteButtonClicked()
{
    // 删除选中的任务
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    for (QGraphicsItem* item : selectedItems) {
        TaskCard* task = dynamic_cast<TaskCard*>(item);
        if (task) {
            m_scene->removeItem(task);
            delete task;
        }
    }
    
    // 重新排列卡片
    arrangeCards();
    
    // 保存到数据库
    saveTasks();
}

void MainWindow::onClearButtonClicked()
{
    // 清空数据库和场景中的所有任务
    QSqlQuery query;
    query.exec("DELETE FROM tasks");
    
    // 清除场景中的所有任务卡片
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem* item : items) {
        if (dynamic_cast<TaskCard*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }
}

void MainWindow::onCardDoubleClicked(TaskCard *card)
{
    // 记录当前正在编辑的卡片
    m_currentEditCard = card;
    
    // 设置对话框标题为编辑任务
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("编辑任务"));
    
    // 填充表单数据
    m_titleEdit->setText(card->title());
    m_descEdit->setText(card->description());
    
    // 设置优先级
    switch (card->priority()) {
        case TaskCard::Low: m_priorityCombo->setCurrentIndex(0); break;
        case TaskCard::Medium: m_priorityCombo->setCurrentIndex(1); break;
        case TaskCard::High: m_priorityCombo->setCurrentIndex(2); break;
    }
    
    // 设置状态
    switch (card->status()) {
        case TaskCard::Todo: m_statusCombo->setCurrentIndex(0); break;
        case TaskCard::InProgress: m_statusCombo->setCurrentIndex(1); break;
        case TaskCard::Done: m_statusCombo->setCurrentIndex(2); break;
    }
    
    // 设置截止日期
    m_deadlineEdit->setDateTime(card->deadline());
    
    // 居中显示对话框
    m_taskDialog->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            m_taskDialog->size(),
            QApplication::desktop()->availableGeometry()
        )
    );
    
    // 执行对话框（模态显示）
    m_taskDialog->exec();
}

void MainWindow::updateCardStatusByPosition(TaskCard *card)
{
    // 获取卡片当前位置
    QPointF pos = card->scenePos();
    
    // 确定卡片应该属于哪个状态列
    TaskCard::Status newStatus = getStatusFromPosition(pos.x());
    
    // 如果状态变化了，更新卡片状态
    if (card->status() != newStatus) {
        card->setStatus(newStatus);
        
        // 重新排列卡片
        arrangeCards();
        
        // 保存到数据库
        saveTasks();
    }
}

void MainWindow::onReportButtonClicked()
{
    // 收集所有任务卡片数据
    QList<TaskCard*> currentCards;
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            currentCards.append(card);
        }
    }

    // 创建并显示报表对话框
    ReportDialog reportDialog(currentCards, this);
    reportDialog.exec(); // 使用 exec() 使其成为模态对话框
}

void MainWindow::setupScene()
{
    // 连接现有卡片的信号
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            // 确保断开旧连接，避免重复连接
            disconnect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
            disconnect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
            // 重新连接
            connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
            connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        }
    }
}