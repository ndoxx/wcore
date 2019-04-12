
## Connecting lambdas

```cpp
class Task : public QWidget
{
// ...
public slots:
    void rename();
signals:
    void removed(Task* task);
// ...
};

Task::Task(const QString& name, QWidget *parent) :
QWidget(parent),
ui(new Ui::Task)
{
    ui->setupUi(this);
    // ...
    connect(ui->removeButton, &QPushButton::clicked, [this] {
        emit removed(this);
        });
}
```
