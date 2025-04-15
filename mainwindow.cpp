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
#include <QWheelEvent>
#include <QScrollBar>
#include <QToolBar>
#include <QSlider>
#include <QAction>
#include <QLabel>
#include <QGraphicsLineItem>
#include "reportdialog.h"
#include <QScreen>

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
      m_scene(nullptr),
      m_assigneeEdit(nullptr),
      m_startDateEdit(nullptr),
      m_endDateEdit(nullptr),
      m_assigneeFilterEdit(nullptr),
      m_zoomFactor(1.0)
{
    ui->setupUi(this);
    showMaximized();
    setWindowTitle(QString::fromLocal8Bit("任务管理系统"));
    
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);
    
    // 启用滚动条
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QLinearGradient gradient(0, 0, 0, ui->graphicsView->height());
    gradient.setColorAt(0, QColor(10, 35, 80));  // 深蓝色渐变起始色
    gradient.setColorAt(1, QColor(5, 15, 40));   // 深蓝色渐变结束色
    m_scene->setBackgroundBrush(gradient);
    
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    setupColumns();
    setupZoomControls();
    setupTaskDialog();
    
    m_startDateEdit = ui->startDateEdit;
    m_endDateEdit = ui->endDateEdit;
    m_assigneeFilterEdit = ui->assigneeFilterEdit;

    m_startDateEdit->setDateTime(QDateTime::currentDateTime().addMonths(-1));
    m_endDateEdit->setDateTime(QDateTime::currentDateTime().addMonths(1));
    
    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteButtonClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::onClearButtonClicked);
    connect(ui->reportButton, &QPushButton::clicked, this, &MainWindow::onReportButtonClicked);
    connect(ui->filterButton, &QPushButton::clicked, this, &MainWindow::onFilterButtonClicked);
    connect(ui->clearFilterButton, &QPushButton::clicked, this, &MainWindow::onClearFilterButtonClicked);
    
    initDatabase();
    loadTasks();
    setupScene();
}

MainWindow::~MainWindow()
{
    saveTasks();
    
    if (m_taskDialog) {
        delete m_taskDialog;
        m_taskDialog = nullptr;
    }
    
    delete ui;
}

void MainWindow::setupColumns()
{
    const int columnWidth = 450;  // 固定列宽
    const int columnHeight = 650;
    const int columnSpacing = 30;
    const int leftMargin = 30;    // 左边距
    
    // 设置场景大小
    m_scene->setSceneRect(0, 0, leftMargin * 2 + columnWidth * 3 + columnSpacing * 2, columnHeight + 100);
    
    QGraphicsTextItem *todoTitle = m_scene->addText(QString::fromLocal8Bit("待办任务"), QFont(QString::fromLocal8Bit("微软雅黑"), 16, QFont::Bold));
    todoTitle->setDefaultTextColor(QColor(220, 220, 220));
    todoTitle->setPos(leftMargin, 30);
    
    QGraphicsTextItem *inProgressTitle = m_scene->addText(QString::fromLocal8Bit("进行中"), QFont(QString::fromLocal8Bit("微软雅黑"), 16, QFont::Bold));
    inProgressTitle->setDefaultTextColor(QColor(220, 220, 220));
    inProgressTitle->setPos(leftMargin + columnWidth + columnSpacing, 30);
    
    QGraphicsTextItem *doneTitle = m_scene->addText(QString::fromLocal8Bit("已完成"), QFont(QString::fromLocal8Bit("微软雅黑"), 16, QFont::Bold));
    doneTitle->setDefaultTextColor(QColor(220, 220, 220));
    doneTitle->setPos(leftMargin + (columnWidth + columnSpacing) * 2, 30);
    
    // 列的边框和背景色
    QPen columnPen(QColor(255, 255, 255, 20));
    QBrush columnBrush(QColor(255, 255, 255, 8));
    
    todoColumn = m_scene->addRect(leftMargin, 70, columnWidth, columnHeight, columnPen, columnBrush);
    inProgressColumn = m_scene->addRect(leftMargin + columnWidth + columnSpacing, 70, columnWidth, columnHeight, columnPen, columnBrush);
    doneColumn = m_scene->addRect(leftMargin + (columnWidth + columnSpacing) * 2, 70, columnWidth, columnHeight, columnPen, columnBrush);
}

void MainWindow::setupTaskDialog()
{
    if (m_taskDialog) {
        delete m_taskDialog;
        m_taskDialog = nullptr;
    }
    
    m_taskDialog = new QDialog(this);
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("创建新任务"));
    m_taskDialog->setMinimumWidth(400);
    
    // 设置对话框背景色
    QPalette dlgPalette = m_taskDialog->palette();
    dlgPalette.setColor(QPalette::Window, QColor(10, 35, 80));
    dlgPalette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    dlgPalette.setColor(QPalette::Base, QColor(15, 45, 90));
    dlgPalette.setColor(QPalette::Text, QColor(220, 220, 220));
    m_taskDialog->setPalette(dlgPalette);
    m_taskDialog->setAutoFillBackground(true);

    m_taskDialog->setModal(true);
    
    m_taskDialog->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::MSWindowsFixedSizeDialogHint);
    
    m_taskDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    
    QSize dialogSize(400, 300);
    m_taskDialog->resize(dialogSize);
    
    QVBoxLayout *layout = new QVBoxLayout(m_taskDialog);
    QFormLayout *formLayout = new QFormLayout();
    
    m_titleEdit = new QLineEdit(m_taskDialog);
    formLayout->addRow(QString::fromLocal8Bit("任务标题:"), m_titleEdit);
    
    m_descEdit = new QTextEdit(m_taskDialog);
    m_descEdit->setMaximumHeight(100);
    formLayout->addRow(QString::fromLocal8Bit("任务描述:"), m_descEdit);
    
    m_priorityCombo = new QComboBox(m_taskDialog);
    m_priorityCombo->addItem(QString::fromLocal8Bit("低"), TaskCard::Low);
    m_priorityCombo->addItem(QString::fromLocal8Bit("中"), TaskCard::Medium);
    m_priorityCombo->addItem(QString::fromLocal8Bit("高"), TaskCard::High);
    m_priorityCombo->setCurrentIndex(1);
    formLayout->addRow(QString::fromLocal8Bit("优先级:"), m_priorityCombo);
    
    m_statusCombo = new QComboBox(m_taskDialog);
    m_statusCombo->addItem(QString::fromLocal8Bit("待办"), TaskCard::Todo);
    m_statusCombo->addItem(QString::fromLocal8Bit("进行中"), TaskCard::InProgress);
    m_statusCombo->addItem(QString::fromLocal8Bit("已完成"), TaskCard::Done);
    m_statusCombo->setCurrentIndex(0);
    formLayout->addRow(QString::fromLocal8Bit("状态:"), m_statusCombo);
    
    m_deadlineEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(7), m_taskDialog);
    m_deadlineEdit->setCalendarPopup(true);
    formLayout->addRow(QString::fromLocal8Bit("截止日期:"), m_deadlineEdit);
    
    m_assigneeEdit = new QLineEdit(m_taskDialog);
    formLayout->addRow(QString::fromLocal8Bit("执行人:"), m_assigneeEdit);
    
    layout->addLayout(formLayout);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *cancelButton = new QPushButton(QString::fromLocal8Bit("取消"), m_taskDialog);
    QPushButton *okButton = new QPushButton(QString::fromLocal8Bit("确定"), m_taskDialog);
    
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    layout->addLayout(buttonLayout);
    
    connect(cancelButton, &QPushButton::clicked, m_taskDialog, &QDialog::reject);
    connect(okButton, &QPushButton::clicked, this, &MainWindow::onTaskDialogAccepted);
    connect(m_taskDialog, &QDialog::rejected, [this]() {
        m_currentEditCard = nullptr;
        m_scene->update();
        ui->graphicsView->viewport()->update();
    });
}

void MainWindow::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("tasks.db");
    
    if (!m_db.open()) {
        qDebug() << "Error: connection with database failed";
    } else {
        qDebug() << "Database: connection ok";
    }
    
    QSqlQuery query;
    
    // 创建任务表，添加id和progress字段
    query.exec("CREATE TABLE IF NOT EXISTS tasks ("
              "id TEXT PRIMARY KEY, "
              "title TEXT, "
              "description TEXT, "
              "status INTEGER, "
              "priority INTEGER, "
              "deadline TEXT, "
              "assignee TEXT, "
              "progress INTEGER, "
              "project_id TEXT)");
    
    // 添加依赖关系表
    query.exec("CREATE TABLE IF NOT EXISTS dependencies ("
              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "task_id TEXT, "
              "dependency_id TEXT)");
}

void MainWindow::saveTasks()
{
    if (!m_db.isOpen()) {
        qDebug() << "Database is not open, cannot save tasks.";
        return;
    }
    
    QSqlQuery query;
    
    // 删除之前的所有任务
    query.exec("DELETE FROM tasks");
    
    // 保存当前所有任务
    for (TaskCard *card : m_cards) {
        query.prepare("INSERT INTO tasks (id, title, description, status, priority, deadline, assignee, progress, project_id) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        
        query.addBindValue(card->id());
        query.addBindValue(card->title());
        query.addBindValue(card->description());
        query.addBindValue(static_cast<int>(card->status()));
        query.addBindValue(static_cast<int>(card->priority()));
        
        if (card->deadline().isValid()) {
            query.addBindValue(card->deadline().toString(Qt::ISODate));
        } else {
            query.addBindValue(QString());
        }
        
        query.addBindValue(card->assignee());
        query.addBindValue(card->progress());
        query.addBindValue(card->projectId());
        
        if (!query.exec()) {
            qDebug() << "Save task error: " << query.lastError().text();
        }
    }
    
    // 保存依赖关系
    QSqlQuery depQuery;
    depQuery.exec("DELETE FROM dependencies");
    
    for (TaskCard* card : m_cards) {
        for (TaskCard* depCard : card->dependencies()) {
            depQuery.prepare("INSERT INTO dependencies (task_id, dependency_id) VALUES (?, ?)");
            depQuery.addBindValue(card->id());
            depQuery.addBindValue(depCard->id());
            depQuery.exec();
        }
    }
}

void MainWindow::loadTasks()
{
    if (!m_db.isOpen()) {
        qDebug() << "Database is not open, cannot load tasks.";
        return;
    }
    
    // 清除现有任务
    for (TaskCard *card : m_cards) {
        m_scene->removeItem(card);
        delete card;
    }
    m_cards.clear();
    
    QSqlQuery query("SELECT id, title, description, status, priority, deadline, assignee, progress, project_id FROM tasks");
    
    // 加载所有任务
    while (query.next()) {
        QString id = query.value(0).toString();
        QString title = query.value(1).toString();
        QString description = query.value(2).toString();
        int statusInt = query.value(3).toInt();
        int priorityInt = query.value(4).toInt();
        QString deadlineStr = query.value(5).toString();
        QString assignee = query.value(6).toString();
        int progress = query.value(7).toInt();
        QString projectId = query.value(8).toString();
        
        QDateTime deadline;
        if (!deadlineStr.isEmpty()) {
            deadline = QDateTime::fromString(deadlineStr, Qt::ISODate);
        }
        
        TaskCard::Status status = static_cast<TaskCard::Status>(statusInt);
        TaskCard::Priority priority = static_cast<TaskCard::Priority>(priorityInt);
        
        TaskCard *card = new TaskCard(title, description, priority, status, deadline, assignee, projectId);
        card->setId(id);
        card->setProgress(progress);
        
        m_scene->addItem(card);
        m_cards.append(card);
        
        // 连接任务卡片的信号
        connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::showTaskDetails);
        connect(card, &TaskCard::cardHovered, this, [this, card]() {
            if (!card->dependencies().isEmpty()) {
                drawDependencyLines(card);
            }
        });
    }
    
    // 在加载所有任务后，设置依赖关系（因为需要先创建所有任务对象）
    QSqlQuery depQuery("SELECT task_id, dependency_id FROM dependencies");
    QMap<QString, TaskCard*> cardMap;
    
    // 构建ID到卡片的映射
    for (TaskCard* card : m_cards) {
        cardMap[card->id()] = card;
    }
    
    // 设置依赖关系
    while (depQuery.next()) {
        QString taskId = depQuery.value(0).toString();
        QString depId = depQuery.value(1).toString();
        
        if (cardMap.contains(taskId) && cardMap.contains(depId)) {
            cardMap[taskId]->addDependency(cardMap[depId]);
        }
    }
    
    arrangeCards();
}

void MainWindow::arrangeCards()
{
    const int cardSpacing = 20;
    qreal todoY = todoColumn->rect().y() + 20;
    qreal inProgressY = inProgressColumn->rect().y() + 20;
    qreal doneY = doneColumn->rect().y() + 20;
    
    QList<QGraphicsItem*> items = m_scene->items();
    QList<TaskCard*> todoCards;
    QList<TaskCard*> inProgressCards;
    QList<TaskCard*> doneCards;

    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card && card->isVisible()) {
            switch (card->status()) {
                case TaskCard::Todo: todoCards.append(card); break;
                case TaskCard::InProgress: inProgressCards.append(card); break;
                case TaskCard::Done: doneCards.append(card); break;
            }
        }
    }

    for (TaskCard *card : todoCards) {
        card->setPos(todoColumn->rect().x() + 25, todoY);
        todoY += card->boundingRect().height() + cardSpacing;
    }
    for (TaskCard *card : inProgressCards) {
        card->setPos(inProgressColumn->rect().x() + 25, inProgressY);
        inProgressY += card->boundingRect().height() + cardSpacing;
    }
    for (TaskCard *card : doneCards) {
        card->setPos(doneColumn->rect().x() + 25, doneY);
        doneY += card->boundingRect().height() + cardSpacing;
    }
}

TaskCard::Status MainWindow::getStatusFromPosition(qreal x)
{
    qreal sceneWidth = m_scene->width();
    qreal columnWidth = sceneWidth / 3;
    
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
    m_currentEditCard = nullptr;
    
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("添加任务"));
    
    m_titleEdit->clear();
    m_descEdit->clear();
    m_priorityCombo->setCurrentIndex(0);
    m_statusCombo->setCurrentIndex(0);
    m_deadlineEdit->setDateTime(QDateTime::currentDateTime().addDays(7));
    m_assigneeEdit->clear();
    
    m_taskDialog->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            m_taskDialog->size(),
            QApplication::desktop()->availableGeometry()
        )
    );
    
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
    
    QString assignee = m_assigneeEdit->text().trimmed();
    
    if (m_currentEditCard) {
        m_currentEditCard->setTitle(title);
        m_currentEditCard->setDescription(description);
        m_currentEditCard->setPriority(priority);
        m_currentEditCard->setStatus(status);
        m_currentEditCard->setDeadline(deadline);
        m_currentEditCard->setAssignee(assignee);
        
        m_currentEditCard = nullptr;
        
        arrangeCards();
    } 
    else {
        createNewTask(title, description, priority, status, deadline, assignee);
    }
    
    saveTasks();
    
    m_taskDialog->accept();
    
    m_scene->update();
}

void MainWindow::createNewTask(const QString &title, const QString &description, 
                           TaskCard::Priority priority, TaskCard::Status status, 
                           const QDateTime &deadline, const QString& assignee)
{
    TaskCard *card = new TaskCard(title, description, priority, status, deadline, assignee);
    
    // 初始进度设置，根据状态自动给予默认值
    if (status == TaskCard::Done) {
        card->setProgress(100);
    } else if (status == TaskCard::InProgress) {
        card->setProgress(50);
    } else {
        card->setProgress(0);
    }
    
    m_scene->addItem(card);
    m_cards.append(card);
    
    // 连接信号槽
    connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
    connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::showTaskDetails);
    connect(card, &TaskCard::cardHovered, this, [this, card]() {
        if (!card->dependencies().isEmpty()) {
            drawDependencyLines(card);
        }
    });
    
    arrangeCards();
}

void MainWindow::onDeleteButtonClicked()
{
    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    for (QGraphicsItem* item : selectedItems) {
        TaskCard* task = dynamic_cast<TaskCard*>(item);
        if (task) {
            m_scene->removeItem(task);
            delete task;
        }
    }
    
    arrangeCards();
    
    saveTasks();
}

void MainWindow::onClearButtonClicked()
{
    QSqlQuery query;
    query.exec("DELETE FROM tasks");
    
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
    if (!card) return;
    
    m_currentEditCard = card;
    
    m_titleEdit->setText(card->title());
    m_descEdit->setText(card->description());
    m_priorityCombo->setCurrentIndex(card->priority());
    m_statusCombo->setCurrentIndex(card->status());
    m_assigneeEdit->setText(card->assignee());
    
    if (card->deadline().isValid()) {
        m_deadlineEdit->setDateTime(card->deadline());
    } else {
        m_deadlineEdit->setDateTime(QDateTime::currentDateTime().addDays(7));
    }
    
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("编辑任务"));
    m_taskDialog->exec();
}

void MainWindow::updateCardStatusByPosition(TaskCard *card)
{
    QPointF pos = card->scenePos();
    
    TaskCard::Status newStatus = getStatusFromPosition(pos.x());
    
    if (card->status() != newStatus) {
        card->setStatus(newStatus);
        
        arrangeCards();
        
        saveTasks();
    }
}

void MainWindow::onReportButtonClicked()
{
    QList<TaskCard*> currentCards;
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            currentCards.append(card);
        }
    }

    ReportDialog reportDialog(currentCards, this);
    reportDialog.exec();
}

void MainWindow::setupScene()
{
    // 设置主窗口背景色
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(5, 15, 40));
    setPalette(pal);
    setAutoFillBackground(true);

    // 设置底部区域的样式
    QString bottomStyle = QString(
        "QWidget {"
        "   background-color: rgb(10, 35, 80);"
        "   color: rgb(220, 220, 220);"
        "}"
        "QLineEdit, QDateTimeEdit {"
        "   background-color: rgb(15, 45, 90);"
        "   border: 1px solid rgb(30, 60, 110);"
        "   color: rgb(220, 220, 220);"
        "   padding: 2px;"
        "}"
        "QPushButton {"
        "   background-color: rgb(24, 110, 165);"
        "   border: 1px solid rgb(41, 128, 185);"
        "   color: rgb(220, 220, 220);"
        "   padding: 5px 15px;"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(41, 128, 185);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(15, 89, 145);"
        "}"
    );
    
    ui->centralWidget->setStyleSheet(bottomStyle);

    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            disconnect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
            disconnect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
            connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
            connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        }
    }

    // 连接任务卡片的悬停信号
    for (TaskCard* card : m_cards) {
        connect(card, &TaskCard::cardHovered, this, [this, card]() {
            // 绘制依赖关系线条
            if (!card->dependencies().isEmpty()) {
                drawDependencyLines(card);
            }
        });
        
        // 连接双击信号到详情页面
        connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::showTaskDetails);
    }
}

void MainWindow::applyFilters()
{
    QDateTime startDate = m_startDateEdit->dateTime();
    QDateTime endDate = m_endDateEdit->dateTime();
    QString assigneeFilter = m_assigneeFilterEdit->text().trimmed();

    for (QGraphicsItem *item : m_scene->items()) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            bool dateMatch = true;
            if (card->deadline().isValid()) {
                dateMatch = (!startDate.isValid() || card->deadline() >= startDate) && 
                            (!endDate.isValid() || card->deadline() <= endDate);
            }
            
            bool assigneeMatch = assigneeFilter.isEmpty() || 
                                 card->assignee().contains(assigneeFilter, Qt::CaseInsensitive);

            card->setVisible(dateMatch && assigneeMatch);
        }
    }
    arrangeCards();
}

void MainWindow::onFilterButtonClicked()
{
    applyFilters();
}

void MainWindow::onClearFilterButtonClicked()
{
    m_startDateEdit->setDateTime(QDateTime::currentDateTime().addMonths(-1));
    m_endDateEdit->setDateTime(QDateTime::currentDateTime().addMonths(1));
    m_assigneeFilterEdit->clear();
    
    for (QGraphicsItem *item : m_scene->items()) {
         TaskCard *card = dynamic_cast<TaskCard*>(item);
         if (card) {
             card->setVisible(true);
         }
     }
    
    arrangeCards();
}

void MainWindow::setupZoomControls()
{
    // 创建缩放工具栏
    QToolBar *zoomToolBar = new QToolBar(QString::fromLocal8Bit("缩放控制"), this);
    addToolBar(Qt::TopToolBarArea, zoomToolBar);
    
    // 添加缩小按钮
    QAction *zoomOutAction = new QAction(QString::fromLocal8Bit("缩小"), this);
    zoomOutAction->setIcon(QIcon::fromTheme("zoom-out"));
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    zoomToolBar->addAction(zoomOutAction);
    
    // 添加缩放滑块
    QSlider *zoomSlider = new QSlider(Qt::Horizontal, this);
    zoomSlider->setRange(50, 200);
    zoomSlider->setValue(100);
    zoomSlider->setToolTip(QString::fromLocal8Bit("缩放级别"));
    connect(zoomSlider, &QSlider::valueChanged, [this](int value) {
        qreal zoomFactor = value / 100.0;
        QTransform transform;
        transform.scale(zoomFactor, zoomFactor);
        ui->graphicsView->setTransform(transform);
        m_zoomFactor = zoomFactor;
    });
    zoomToolBar->addWidget(zoomSlider);
    
    // 添加放大按钮
    QAction *zoomInAction = new QAction(QString::fromLocal8Bit("放大"), this);
    zoomInAction->setIcon(QIcon::fromTheme("zoom-in"));
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    zoomToolBar->addAction(zoomInAction);
    
    // 添加重置缩放按钮
    QAction *resetZoomAction = new QAction(QString::fromLocal8Bit("重置缩放"), this);
    resetZoomAction->setIcon(QIcon::fromTheme("zoom-original"));
    connect(resetZoomAction, &QAction::triggered, this, &MainWindow::resetZoom);
    zoomToolBar->addAction(resetZoomAction);
}

void MainWindow::zoomIn()
{
    if (m_zoomFactor < MAX_ZOOM) {
        m_zoomFactor += ZOOM_FACTOR_STEP;
        QTransform transform;
        transform.scale(m_zoomFactor, m_zoomFactor);
        ui->graphicsView->setTransform(transform);
    }
}

void MainWindow::zoomOut()
{
    if (m_zoomFactor > MIN_ZOOM) {
        m_zoomFactor -= ZOOM_FACTOR_STEP;
        QTransform transform;
        transform.scale(m_zoomFactor, m_zoomFactor);
        ui->graphicsView->setTransform(transform);
    }
}

void MainWindow::resetZoom()
{
    m_zoomFactor = 1.0;
    ui->graphicsView->resetTransform();
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    // 按住Ctrl键时滚轮控制缩放
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QMainWindow::wheelEvent(event);
    }
}

void MainWindow::showTaskDetails(TaskCard* card)
{
    if (!card) return;
    
    QDialog *detailsDialog = new QDialog(this);
    detailsDialog->setWindowTitle(QString::fromLocal8Bit("任务详情"));
    detailsDialog->setMinimumSize(450, 350);
    
    // 设置对话框背景色
    QPalette dlgPalette = detailsDialog->palette();
    dlgPalette.setColor(QPalette::Window, QColor(30, 50, 90));
    dlgPalette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    detailsDialog->setPalette(dlgPalette);
    detailsDialog->setAutoFillBackground(true);
    
    QVBoxLayout *layout = new QVBoxLayout(detailsDialog);
    
    // 标题
    QLabel *titleLabel = new QLabel(QString("<h2>%1</h2>").arg(card->title()), detailsDialog);
    titleLabel->setStyleSheet("color: white;");
    layout->addWidget(titleLabel);
    
    // 状态和优先级
    QString status;
    switch (card->status()) {
        case TaskCard::Todo: status = QString::fromLocal8Bit("待办"); break;
        case TaskCard::InProgress: status = QString::fromLocal8Bit("进行中"); break;
        case TaskCard::Done: status = QString::fromLocal8Bit("已完成"); break;
    }
    
    QString priority;
    switch (card->priority()) {
        case TaskCard::Low: priority = QString::fromLocal8Bit("低"); break;
        case TaskCard::Medium: priority = QString::fromLocal8Bit("中"); break;
        case TaskCard::High: priority = QString::fromLocal8Bit("高"); break;
    }
    
    QLabel *metaLabel = new QLabel(QString::fromLocal8Bit("状态: %1 | 优先级: %2").arg(status).arg(priority), detailsDialog);
    metaLabel->setStyleSheet("color: #cccccc;");
    layout->addWidget(metaLabel);
    
    // 截止日期
    if (card->deadline().isValid()) {
        QLabel *deadlineLabel = new QLabel(QString::fromLocal8Bit("截止日期: %1").arg(card->deadline().toString("yyyy-MM-dd")), detailsDialog);
        deadlineLabel->setStyleSheet("color: #cccccc;");
        layout->addWidget(deadlineLabel);
    }
    
    // 执行人
    if (!card->assignee().isEmpty()) {
        QLabel *assigneeLabel = new QLabel(QString::fromLocal8Bit("执行人: %1").arg(card->assignee()), detailsDialog);
        assigneeLabel->setStyleSheet("color: #cccccc;");
        layout->addWidget(assigneeLabel);
    }
    
    // 进度
    QLabel *progressLabel = new QLabel(QString::fromLocal8Bit("完成进度: %1%").arg(card->progress()), detailsDialog);
    progressLabel->setStyleSheet("color: #cccccc;");
    layout->addWidget(progressLabel);
    
    // 进度条
    QSlider *progressSlider = new QSlider(Qt::Horizontal, detailsDialog);
    progressSlider->setRange(0, 100);
    progressSlider->setValue(card->progress());
    layout->addWidget(progressSlider);
    
    // 任务描述
    QLabel *descriptionTitle = new QLabel(QString::fromLocal8Bit("<h3>任务描述:</h3>"), detailsDialog);
    descriptionTitle->setStyleSheet("color: white;");
    layout->addWidget(descriptionTitle);
    
    QTextEdit *descriptionEdit = new QTextEdit(detailsDialog);
    descriptionEdit->setText(card->description());
    descriptionEdit->setStyleSheet("background-color: rgba(255, 255, 255, 0.1); color: white; border: 1px solid rgba(255, 255, 255, 0.2);");
    descriptionEdit->setReadOnly(true);
    layout->addWidget(descriptionEdit);
    
    // 依赖关系列表
    QLabel *depsTitle = new QLabel(QString::fromLocal8Bit("<h3>依赖任务:</h3>"), detailsDialog);
    depsTitle->setStyleSheet("color: white;");
    layout->addWidget(depsTitle);
    
    QListWidget *depsList = new QListWidget(detailsDialog);
    depsList->setStyleSheet("background-color: rgba(255, 255, 255, 0.1); color: white; border: 1px solid rgba(255, 255, 255, 0.2);");
    for (const TaskCard* depCard : card->dependencies()) {
        depsList->addItem(depCard->title());
    }
    if (card->dependencies().isEmpty()) {
        depsList->addItem(QString::fromLocal8Bit("无依赖任务"));
    }
    layout->addWidget(depsList);
    
    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    QPushButton *editDepsButton = new QPushButton(QString::fromLocal8Bit("管理依赖"), detailsDialog);
    editDepsButton->setStyleSheet("background-color: #5bc0de; color: white; border: none; padding: 5px 10px;");
    connect(editDepsButton, &QPushButton::clicked, [this, card, detailsDialog]() {
        manageDependencies(card);
        detailsDialog->accept(); // 关闭当前对话框
    });
    btnLayout->addWidget(editDepsButton);
    
    QPushButton *editButton = new QPushButton(QString::fromLocal8Bit("编辑任务"), detailsDialog);
    editButton->setStyleSheet("background-color: #337ab7; color: white; border: none; padding: 5px 10px;");
    connect(editButton, &QPushButton::clicked, [this, card, detailsDialog]() {
        onCardDoubleClicked(card);
        detailsDialog->accept();
    });
    btnLayout->addWidget(editButton);
    
    QPushButton *closeButton = new QPushButton(QString::fromLocal8Bit("关闭"), detailsDialog);
    closeButton->setStyleSheet("background-color: #5c5c5c; color: white; border: none; padding: 5px 10px;");
    connect(closeButton, &QPushButton::clicked, detailsDialog, &QDialog::accept);
    btnLayout->addWidget(closeButton);
    
    // 更新进度
    connect(progressSlider, &QSlider::valueChanged, [card, progressLabel](int value) {
        card->setProgress(value);
        progressLabel->setText(QString::fromLocal8Bit("完成进度: %1%").arg(value));
    });
    
    layout->addLayout(btnLayout);
    
    detailsDialog->setLayout(layout);
    detailsDialog->exec();
}

// 绘制依赖关系线条
void MainWindow::drawDependencyLines(TaskCard* card)
{
    // 清除之前的依赖线
    QList<QGraphicsItem*> allItems = m_scene->items();
    for (QGraphicsItem* item : allItems) {
        if (item->data(0).toString() == "dependency_line") {
            m_scene->removeItem(item);
            delete item;
        }
    }
    
    // 为每个依赖绘制一条线
    for (TaskCard* depCard : card->dependencies()) {
        QGraphicsLineItem* line = new QGraphicsLineItem();
        line->setData(0, "dependency_line");
        
        // 设置线条样式
        QPen pen(QColor(120, 180, 255, 180), 2, Qt::DashLine); // 虚线
        line->setPen(pen);
        
        // 设置线条起止点
        QPointF startPoint = card->pos() + QPointF(card->boundingRect().width() / 2, card->boundingRect().height() / 2);
        QPointF endPoint = depCard->pos() + QPointF(depCard->boundingRect().width() / 2, depCard->boundingRect().height() / 2);
        line->setLine(QLineF(startPoint, endPoint));
        
        // 添加箭头
        QPolygonF arrowHead;
        qreal arrowSize = 10.0;
        QLineF lineDirection(endPoint, startPoint);
        lineDirection.setLength(arrowSize);
        QPointF arrowP1 = endPoint + QPointF(lineDirection.dx() + lineDirection.dy() * 0.4, lineDirection.dy() - lineDirection.dx() * 0.4);
        QPointF arrowP2 = endPoint + QPointF(lineDirection.dx() - lineDirection.dy() * 0.4, lineDirection.dy() + lineDirection.dx() * 0.4);
        arrowHead << endPoint << arrowP1 << arrowP2;
        
        QGraphicsPolygonItem* arrowItem = m_scene->addPolygon(arrowHead, Qt::NoPen, QBrush(QColor(120, 180, 255, 180)));
        arrowItem->setData(0, "dependency_line");
        
        m_scene->addItem(line);
    }
}

// 添加管理依赖关系的方法
void MainWindow::manageDependencies(TaskCard* card)
{
    if (!card) return;
    
    QDialog *depDialog = new QDialog(this);
    depDialog->setWindowTitle(QString::fromLocal8Bit("管理依赖关系"));
    depDialog->setMinimumSize(400, 300);
    
    // 设置对话框背景色
    QPalette dlgPalette = depDialog->palette();
    dlgPalette.setColor(QPalette::Window, QColor(30, 50, 90));
    dlgPalette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    depDialog->setPalette(dlgPalette);
    depDialog->setAutoFillBackground(true);
    
    QVBoxLayout *layout = new QVBoxLayout(depDialog);
    
    QLabel *titleLabel = new QLabel(QString::fromLocal8Bit("为任务 \"%1\" 管理依赖关系").arg(card->title()), depDialog);
    titleLabel->setStyleSheet("color: white; font-weight: bold;");
    layout->addWidget(titleLabel);
    
    // 显示所有可选任务（除了当前任务）
    QListWidget *taskList = new QListWidget(depDialog);
    taskList->setStyleSheet("background-color: rgba(255, 255, 255, 0.1); color: white; border: 1px solid rgba(255, 255, 255, 0.2);");
    taskList->setSelectionMode(QAbstractItemView::MultiSelection);
    
    QList<TaskCard*> dependencies = card->dependencies();
    
    for (TaskCard* otherCard : m_cards) {
        if (otherCard != card) {  // 排除当前任务自身
            QListWidgetItem *item = new QListWidgetItem(otherCard->title(), taskList);
            item->setData(Qt::UserRole, QVariant::fromValue(otherCard));
            
            // 如果是已有依赖，则预先选中
            if (dependencies.contains(otherCard)) {
                item->setSelected(true);
            }
        }
    }
    
    layout->addWidget(taskList);
    
    QLabel *infoLabel = new QLabel(QString::fromLocal8Bit("选择的任务将成为当前任务的依赖项。"));
    infoLabel->setStyleSheet("color: #cccccc;");
    layout->addWidget(infoLabel);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton(QString::fromLocal8Bit("确定"), depDialog);
    okButton->setStyleSheet("background-color: #337ab7; color: white; border: none; padding: 5px 10px;");
    QPushButton *cancelButton = new QPushButton(QString::fromLocal8Bit("取消"), depDialog);
    cancelButton->setStyleSheet("background-color: #5c5c5c; color: white; border: none; padding: 5px 10px;");
    
    btnLayout->addWidget(cancelButton);
    btnLayout->addWidget(okButton);
    layout->addLayout(btnLayout);
    
    connect(cancelButton, &QPushButton::clicked, depDialog, &QDialog::reject);
    connect(okButton, &QPushButton::clicked, [this, depDialog, taskList, card]() {
        // 清除现有依赖
        QList<TaskCard*> oldDependencies = card->dependencies();
        for (TaskCard* dep : oldDependencies) {
            card->removeDependency(dep);
        }
        
        // 添加新选择的依赖
        QList<QListWidgetItem*> selectedItems = taskList->selectedItems();
        for (QListWidgetItem* item : selectedItems) {
            TaskCard* depCard = item->data(Qt::UserRole).value<TaskCard*>();
            card->addDependency(depCard);
        }
        
        // 更新视图
        drawDependencyLines(card);
        m_scene->update();
        
        depDialog->accept();
    });
    
    depDialog->exec();
}

// 移除 resizeEvent，因为不再需要自适应布局
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}