#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QScrollArea>  // 添加滚动区域支持
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

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;  // 添加鼠标滚轮事件处理

public slots:
    // 处理卡片拖放，根据位置更新状态
    void updateCardStatusByPosition(TaskCard *card);
    // 添加缩放槽函数
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    //Ui_MainWindow* ui;
    Ui::MainWindow* ui;
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
    QLineEdit *m_assigneeEdit; // Add assignee line edit
    
    // 当前正在编辑的卡片
    TaskCard *m_currentEditCard;
    
    // 筛选组件 (assuming added in UI file)
    QDateTimeEdit *m_startDateEdit;
    QDateTimeEdit *m_endDateEdit;
    QLineEdit *m_assigneeFilterEdit;
    
    // 缩放相关
    qreal m_zoomFactor;
    const qreal ZOOM_FACTOR_STEP = 0.1;
    const qreal MIN_ZOOM = 0.5;
    const qreal MAX_ZOOM = 2.0;
    
    void initDatabase();
    void saveTasks();
    void loadTasks();
    void setupColumns();
    void setupTaskDialog();
    void setupScene();
    void setupZoomControls(); // 添加缩放控制设置
    
    // 创建新任务
    void createNewTask(const QString &title, const QString &description, 
                     TaskCard::Priority priority, TaskCard::Status status, 
                     const QDateTime &deadline, const QString& assignee);
    
    // 自动排列任务卡片
    void arrangeCards();
    
    // 确定任务卡片位置属于哪一列
    TaskCard::Status getStatusFromPosition(qreal x);

    void applyFilters(); // Add filter application method
    
    // 显示任务详情对话框
    void showTaskDetails(TaskCard* card);
    
    // 绘制依赖关系线条
    void drawDependencyLines(TaskCard* card);
    
    // 管理任务依赖关系
    void manageDependencies(TaskCard* card);

private slots:
    void onAddButtonClicked();
    void onDeleteButtonClicked();
    void onClearButtonClicked();
    void onReportButtonClicked();
    void onTaskDialogAccepted();
    
    // 处理卡片双击事件
    void onCardDoubleClicked(TaskCard *card);
    void onFilterButtonClicked(); // Slot for filter button
    void onClearFilterButtonClicked(); // Slot for clear filter button
};

#endif // MAINWINDOW_H