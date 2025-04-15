#ifndef TASKCARD_H
#define TASKCARD_H

#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QLabel>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <QList>
#include <QListWidget>
#include <qabstractitemview.h>

class TaskCard : public QGraphicsWidget
{
    Q_OBJECT

public:
    enum Status { Todo, InProgress, Done };
    enum Priority { Low, Medium, High };
    
    explicit TaskCard(const QString &title, const QString &description, 
                     Priority priority = Medium, 
                     Status status = Todo,
                     const QDateTime &deadline = QDateTime(), 
                     const QString &assignee = QString(),
                     const QString &projectId = QString(),
                     QGraphicsItem *parent = nullptr);

    void setTitle(const QString &title);
    QString title() const;
    void setDescription(const QString &description);
    QString description() const;
    void setStatus(Status status);
    Status status() const;
    void setPriority(Priority priority);
    Priority priority() const;
    void setDeadline(const QDateTime &deadline);
    QDateTime deadline() const;
    bool isSelected() const;
    void setAssignee(const QString &assignee);
    QString assignee() const;
    
    // 进度相关
    void setProgress(int progress);
    int progress() const;
    
    // 依赖关系相关
    void addDependency(TaskCard* task);
    void removeDependency(TaskCard* task);
    QList<TaskCard*> dependencies() const;
    
    // ID相关
    QString id() const;
    void setId(const QString &id);
    
    // 项目ID相关
    QString projectId() const;
    void setProjectId(const QString &projectId);
    
    // 设置颜色和字体
    void setCardColor(const QColor &color);
    void setTitleFont(const QFont &font);
    void setTextFont(const QFont &font);
    
    // 导出任务为JSON格式
    QString toJson() const;

signals:
    void cardDoubleClicked(TaskCard* card);
    void cardReleased(TaskCard* card);
    void cardHovered(TaskCard* card);
    void progressChanged(TaskCard* card, int progress);
    void ganttChartRequested(const QString &projectId);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    QString m_id;
    QString m_title;
    QString m_description;
    Status m_status;
    Priority m_priority;
    QDateTime m_deadline;
    QString m_assignee;
    QString m_projectId;
    QGraphicsLinearLayout *m_layout;
    QPointF m_dragStartPos;
    bool m_selected;
    qreal m_opacity;  // 透明度属性
    
    // 进度相关
    int m_progress;  // 0-100
    
    // 依赖相关
    QList<TaskCard*> m_dependencies;
    
    // 自定义样式
    QColor m_customColor;
    QFont m_titleFont;
    QFont m_textFont;
    
    // 添加发光效果相关成员
    QTimer *m_glowTimer;
    qreal m_glowIntensity;
    bool m_glowIncreasing;

    // 根据优先级获取颜色
    QColor getPriorityColor() const;
    // 渲染任务卡片
    void renderCardContent(QPainter *painter, const QRectF &rect);
    void drawGlowingText(QPainter *painter, const QRectF &rect, const QString &text, 
                        const QFont &font, const QColor &color, Qt::Alignment alignment = Qt::AlignLeft);
                        
    // 绘制进度条
    void drawProgressBar(QPainter *painter, const QRectF &rect);

private slots:
    void updateGlowEffect();
};

#endif // TASKCARD_H