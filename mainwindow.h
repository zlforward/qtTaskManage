#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include "taskcard.h"
#include "reportdialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    // 处理卡片拖放，根据位置更新状态
    void updateCardStatusByPosition(TaskCard *card);

private:
    Ui::MainWindow *ui;
    QGraphicsView *view;
    QGraphicsScene *m_scene;
    QSqlDatabase m_db;
    QGraphicsRectItem *todoColumn;
    QGraphicsRectItem *inProgressColumn;
    QGraphicsRectItem *doneColumn;
    
    // 任务卡片列表
    QList<TaskCard*> m_cards;
    
    // 创建任务对话框组件
    QDialog *m_taskDialog;
    QLineEdit *m_titleEdit;
    QTextEdit *m_descEdit;
    QComboBox *m_priorityCombo;
    QDateTimeEdit *m_deadlineEdit;
    QComboBox *m_statusCombo; // 新增状态选择下拉框
    
    // 当前正在编辑的卡片
    TaskCard *m_currentEditCard;
    
    void initDatabase();
    void saveTasks();
    void loadTasks();
    void setupColumns();
    void setupTaskDialog();
    void setupScene();
    
    // 创建新任务
    void createNewTask(const QString &title, const QString &description, 
                     TaskCard::Priority priority, TaskCard::Status status, 
                     const QDateTime &deadline);
    
    // 自动排列任务卡片
    void arrangeCards();
    
    // 确定任务卡片位置属于哪一列
    TaskCard::Status getStatusFromPosition(qreal x);

private slots:
    void onAddButtonClicked();
    void onDeleteButtonClicked();
    void onClearButtonClicked();
    void onReportButtonClicked();
    void onTaskDialogAccepted();
    
    // 处理卡片双击事件
    void onCardDoubleClicked(TaskCard *card);
};

#endif // MAINWINDOW_H