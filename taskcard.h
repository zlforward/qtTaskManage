#ifndef TASKCARD_H
#define TASKCARD_H

#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QLabel>
#include <QString>
#include <QDateTime>
#include <QTimer>

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
    
    // 导出任务为JSON格式
    QString toJson() const;

signals:
    void cardDoubleClicked(TaskCard* card);
    void cardReleased(TaskCard* card);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QString m_title;
    QString m_description;
    Status m_status;
    Priority m_priority;
    QDateTime m_deadline;
    QString m_assignee;
    QGraphicsLinearLayout *m_layout;
    QPointF m_dragStartPos;
    bool m_selected;
    qreal m_opacity;  // 透明度属性

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

private slots:
    void updateGlowEffect();
};

#endif // TASKCARD_H