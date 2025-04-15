#ifndef TASKCARD_H
#define TASKCARD_H

#include <QGraphicsWidget>
#include <QGraphicsLinearLayout>
#include <QLabel>
#include <QString>
#include <QDateTime>

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
    QGraphicsLinearLayout *m_layout;
    QPointF m_dragStartPos;
    bool m_selected;
    qreal m_opacity;  // 透明度属性

    // 根据优先级获取颜色
    QColor getPriorityColor() const;
    // 渲染任务卡片
    void renderCardContent(QPainter *painter, const QRectF &rect);
};

#endif // TASKCARD_H