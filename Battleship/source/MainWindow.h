#include <QMainWindow>
#include <QTcpSocket>

#include <QStandardItemModel>
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

    public:

          MainWindow(QWidget *parent=0);

    private slots:
        void on_connectButton_clicked();
        void on_sendButton_clicked();
        void on_startButton_clicked();
        void readyRead();
        void connected();
        void shootOnTarget(QModelIndex);
        void showDirections(QModelIndex);

private:
        QTcpSocket *socket;
        QStandardItemModel *myFieldModel;
        QStandardItemModel *enemyFieldModel;
        int cols,rows;
        short semaphore; // ship arrangement lock
        int initRow, initCol, initTop, initDown, initLeft, initRight;
        QModelIndex initIndex;
        short shipsSemaphore;  // to keep track of available ships
        short shipBlocks; // number of occupied blocks for all ships
        bool turn;

        void showMessage(QString message);
        void setTableItem(int,int,QString,QColor,QTableView*,QStandardItemModel*);
        void selectDirections(QModelIndex, int,QColor);
        void addShip(int,int);
        void shotFromEnemy(QString);
        void showResult(QString);
};
