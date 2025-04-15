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
      m_assigneeFilterEdit(nullptr)
{
    ui->setupUi(this);
    showMaximized();
    setWindowTitle(QString::fromLocal8Bit("任务管理系统"));
    
    m_scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(m_scene);
   // ui->graphicsView->setSceneRect(0, 0, 3200, 1600);
    
    QLinearGradient gradient(0, 0, 0, ui->graphicsView->height());
    gradient.setColorAt(0, QColor(10, 35, 80));  // 深蓝色渐变起始色
    gradient.setColorAt(1, QColor(5, 15, 40));   // 深蓝色渐变结束色
    m_scene->setBackgroundBrush(gradient);
    
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    
    setupColumns();
    
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
              "deadline TEXT, "
              "assignee TEXT)");
}

void MainWindow::saveTasks()
{
    QSqlQuery query;
    query.exec("DELETE FROM tasks");
    
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        TaskCard *card = dynamic_cast<TaskCard*>(item);
        if (card) {
            query.prepare("INSERT INTO tasks (title, description, status, priority, deadline, assignee) "
                         "VALUES (:title, :description, :status, :priority, :deadline, :assignee)");
            query.bindValue(":title", card->title());
            query.bindValue(":description", card->description());
            query.bindValue(":status", card->status());
            query.bindValue(":priority", card->priority());
            
            if (card->deadline().isValid()) {
                query.bindValue(":deadline", card->deadline().toString(Qt::ISODate));
            } else {
                query.bindValue(":deadline", QVariant());
            }
            
            query.bindValue(":assignee", card->assignee());
            
            query.exec();
        }
    }
}

void MainWindow::loadTasks()
{
    qDebug() << "Loading tasks from database";
    
    QList<QGraphicsItem*> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (dynamic_cast<TaskCard*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_cards.clear();
    
    QSqlQuery query("SELECT title, description, status, priority, deadline, assignee FROM tasks");
    
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
        
        QString assignee = query.value(5).toString();
        
        TaskCard *card = new TaskCard(title, description, priority, status, deadline, assignee);
        connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        
        m_scene->addItem(card);
        m_cards.append(card);
        count++;
    }
    
    qDebug() << "Loaded " << count << " tasks";
    
    if (count == 0) {
        TaskCard *todo1 = new TaskCard(QString::fromLocal8Bit("完成项目计划"), 
                                      QString::fromLocal8Bit("制定详细的项目实施计划，包括时间表和资源分配"), 
                                      TaskCard::Medium, TaskCard::Todo, QDateTime::currentDateTime().addDays(2), QString::fromLocal8Bit("张三"));
        connect(todo1, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(todo1, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(todo1);
        m_cards.append(todo1);
        
        TaskCard *todo2 = new TaskCard(QString::fromLocal8Bit("学习Qt编程"), 
                                      QString::fromLocal8Bit("完成Qt GUI编程的基础学习，掌握信号与槽机制"), 
                                      TaskCard::High, TaskCard::Todo, QDateTime::currentDateTime().addDays(7), QString::fromLocal8Bit("李四"));
        connect(todo2, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(todo2, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(todo2);
        m_cards.append(todo2);
        
        TaskCard *inProgress = new TaskCard(QString::fromLocal8Bit("开发任务管理系统"), 
                                           QString::fromLocal8Bit("实现基本的任务管理功能，包括创建、编辑和删除任务"), 
                                           TaskCard::High, TaskCard::InProgress, QDateTime::currentDateTime().addDays(5), QString::fromLocal8Bit("王五"));
        connect(inProgress, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(inProgress, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(inProgress);
        m_cards.append(inProgress);
        
        TaskCard *done = new TaskCard(QString::fromLocal8Bit("需求分析"), 
                                     QString::fromLocal8Bit("分析用户需求，确定系统功能和界面设计"), 
                                     TaskCard::Medium, TaskCard::Done, QDateTime::currentDateTime().addDays(-2), QString::fromLocal8Bit("张三"));
        connect(done, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
        connect(done, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
        m_scene->addItem(done);
        m_cards.append(done);
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
                             const QDateTime &deadline, const QString &assignee)
{
    TaskCard *card = new TaskCard(title, description, priority, status, deadline, assignee);
    
    connect(card, &TaskCard::cardDoubleClicked, this, &MainWindow::onCardDoubleClicked);
    connect(card, &TaskCard::cardReleased, this, &MainWindow::updateCardStatusByPosition);
    
    m_scene->addItem(card);
    m_cards.append(card);
    
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
    m_currentEditCard = card;
    
    m_taskDialog->setWindowTitle(QString::fromLocal8Bit("编辑任务"));
    
    m_titleEdit->setText(card->title());
    m_descEdit->setText(card->description());
    
    switch (card->priority()) {
        case TaskCard::Low: m_priorityCombo->setCurrentIndex(0); break;
        case TaskCard::Medium: m_priorityCombo->setCurrentIndex(1); break;
        case TaskCard::High: m_priorityCombo->setCurrentIndex(2); break;
    }
    
    switch (card->status()) {
        case TaskCard::Todo: m_statusCombo->setCurrentIndex(0); break;
        case TaskCard::InProgress: m_statusCombo->setCurrentIndex(1); break;
        case TaskCard::Done: m_statusCombo->setCurrentIndex(2); break;
    }
    
    m_deadlineEdit->setDateTime(card->deadline());
    
    m_assigneeEdit->setText(card->assignee());
    
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

// 移除 resizeEvent，因为不再需要自适应布局
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}