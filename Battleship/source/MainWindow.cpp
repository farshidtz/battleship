#include "MainWindow.h"
#include <QTableWidget>

#include <QRegExp>
#include <QRadioButton>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
    //stackedWidget->setCurrentWidget(mainPage);
    stackedWidget->setCurrentWidget(arrangePage);
    enemyField->setDisabled(true);
    targetLineEdit->setReadOnly(true);

    cols=10,rows=10;
    semaphore = 0;
    shipsSemaphore = 6; // available ships
    shipBlocks = 2*4 + 2*3 + 2*2; // 18
    turn = false;

    myFieldModel = new QStandardItemModel(cols,rows,this); //10 Rows and 10 Columns
    enemyFieldModel = new QStandardItemModel(cols,rows,this); //10 Rows and 10 Columns


    for(int c=0;c<cols;c++)
    {
        myFieldModel->setHorizontalHeaderItem(c, new QStandardItem(QString::number(c+1)));
        enemyFieldModel->setHorizontalHeaderItem(c, new QStandardItem(QString::number(c+1)));
    }
    for(char r='A';r<='J';r++)
    {
        myFieldModel->setVerticalHeaderItem(r-'A', new QStandardItem(QString(r)));
        enemyFieldModel->setVerticalHeaderItem(r-'A', new QStandardItem(QString(r)));
    }

    myField->setModel(myFieldModel);
    enemyField->setModel(enemyFieldModel);
    initialField->setModel(myFieldModel);

    int w_h = 22;

    for(int c=0;c<cols;c++)
    {
        myField->setColumnWidth(c,w_h);
        enemyField->setColumnWidth(c,w_h);
        initialField->setColumnWidth(c,w_h);
    }
    for(int r=0;r<rows;r++)
    {
        myField->setRowHeight(r,w_h);
        enemyField->setRowHeight(r,w_h);
        initialField->setRowHeight(r,w_h);
    }

    myField->setFixedWidth(myField->horizontalHeader()->length()+w_h);
    myField->setFixedHeight(myField->verticalHeader()->length()+w_h+5);
    enemyField->setFixedWidth(myField->horizontalHeader()->length()+w_h);
    enemyField->setFixedHeight(myField->verticalHeader()->length()+w_h+5);
    initialField->setFixedWidth(initialField->horizontalHeader()->length()+w_h);
    initialField->setFixedHeight(initialField->verticalHeader()->length()+w_h+5);


    new QListWidgetItem(QPixmap(":/ship4.png"), "Battleship", shipsListWidget);
    new QListWidgetItem(QPixmap(":/ship4.png"), "Battleship", shipsListWidget);
    new QListWidgetItem(QPixmap(":/ship3.png"), "Destroyer", shipsListWidget);
    new QListWidgetItem(QPixmap(":/ship3.png"), "Destroyer", shipsListWidget);
    new QListWidgetItem(QPixmap(":/ship2.png"), "Patrol Boat", shipsListWidget);
    new QListWidgetItem(QPixmap(":/ship2.png"), "Patrol Boat", shipsListWidget);

    char buffer[100];
    gethostname(buffer,100);
    serverLineEdit->setText(buffer);

    // instantiate the socket
    socket = new QTcpSocket(this);

    // connect on signals from socket
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(socket, SIGNAL(connected()), this, SLOT(connected()));

    // connect on signals from tables
    connect(enemyField, SIGNAL(clicked(QModelIndex)), this, SLOT(shootOnTarget(QModelIndex)));
    connect(initialField, SIGNAL(clicked(QModelIndex)), this, SLOT(showDirections(QModelIndex)));

}

void MainWindow::on_connectButton_clicked()
{
    int port = portSpinBox->value();
    socket->connectToHost(serverLineEdit->text(), port); // connect to host port
}

void MainWindow::on_sendButton_clicked()
{
    QString message = sayLineEdit->text().trimmed(); // remove extra spaces

    if(!message.isEmpty()) // only send if it's not empty
        socket->write(QString(message + "\n").toUtf8());

    sayLineEdit->clear();
    sayLineEdit->setFocus();
}

void MainWindow::readyRead() // gets called when socket has a message
{
    while(socket->canReadLine())
    {
         QString line = QString::fromUtf8(socket->readLine()).trimmed();

        // only one socket is used, so have to differentite different messages
        QRegExp messageRegex("^([^:]+):(.*)$"); //  messges pattern: "username:text of message"
        QRegExp targetRegex("^([^:]+)->(.*)$"); //  target  pattern: "user->A0"
        QRegExp resultRegex("^([^:]+)<-(.*)$"); //  result pattern:  "user<-A00"
        QRegExp usersRegex("^/users:(.*)$");

        if(usersRegex.indexIn(line) != -1) // user connected
        {
            QStringList users = usersRegex.cap(1).split(",");
            userListWidget->clear();
            foreach(QString user, users)
            {
                new QListWidgetItem(QPixmap(":/user.png"), user, userListWidget); // add the new user to the list
                if(user!=userLineEdit->text()) // second user has entered the game
                    enemyField->setDisabled(false);
            }
        }
        else if(messageRegex.indexIn(line) != -1)  // chat message passing
        {
            QString user = messageRegex.cap(1);
            QString message = messageRegex.cap(2);

            roomTextEdit->append("<b>" + user + "</b>: " + message);
        }
        else if(targetRegex.indexIn(line) != -1)// target shooting
        {
            QString user = targetRegex.cap(1);
            QString target = targetRegex.cap(2);

            if(user!=userLineEdit->text()) // enemy->me / me->enemy  NOT  me->me / enemy->enemy
                shotFromEnemy(target);
        }
        else if(resultRegex.indexIn(line) != -1)// receive results
        {
            QString user = resultRegex.cap(1);
            QString result = resultRegex.cap(2);

            if(user!=userLineEdit->text()) // enemy->me / me->enemy  NOT  me->me / enemy->enemy
            {
                if(result=="lost")
                {
                    showMessage("Congradulation!!<br>You Won The Battle.");
                }
                else
                    showResult(result);
            }
        }
    }
}

void MainWindow::connected() // gets called when socket is connected to server
{
    socket->write(QString("/me:" + userLineEdit->text() + "\n").toUtf8());
}

void MainWindow::shotFromEnemy(QString target)
{
    //enemyField->setEnabled(true);
    if(turn==true)
    {
        int row = target.at(0).toAscii()-65; // convert char A-J to 0-9
        int col = target.at(1).digitValue(); // 0-9
        roomTextEdit->append("<i>Block " + QString(target.at(0)) + QString::number(col+1) + " was shot!</i>");
        int hit = 0;
        if(myFieldModel->item(row,col)==0) // null cell
            setTableItem(row,col,"",Qt::blue,myField,myFieldModel);
        else if(myFieldModel->item(row,col)->text().isEmpty()) // empty cell (empty string)
            setTableItem(row,col,"",Qt::blue,myField,myFieldModel);
        else // (item == "-") // a ship block was hit
        {
            setTableItem(row,col,"X",Qt::red,myField,myFieldModel);
            hit = 1;
            shipBlocks--;
            shipsLabel->setText( "(" + QString::number(shipBlocks) + ")" );
        }

        socket->write(QString("/result:" + target + QString::number(hit) +"\n").toUtf8());
        if(shipBlocks==0)
        {
            socket->write(QString("/result:lost\n").toUtf8());
            showMessage("You have lost the battle.\n");
        }
    }
    turn = false;
}

void MainWindow::shootOnTarget(QModelIndex index)
{
    if(turn==false)
    {
        int row = index.row();
        int col = index.column();

        QString target = char(row+65) + QString::number(col);
        targetLineEdit->setText(target);

        socket->write(QString("/target:" + target + "\n").toUtf8());
    }
    turn = true;
}

void MainWindow::showResult(QString result)
{
    int row = result.at(0).toAscii()-65; // convert char A-J to 0-9
    int col = result.at(1).digitValue(); // 0-9
    int hit = result.at(2).digitValue(); // 0 = miss , 1 = hit

    if(hit==0)
        setTableItem(row,col," ",Qt::blue,enemyField,enemyFieldModel);
    else
        setTableItem(row,col,"X",Qt::red,enemyField,enemyFieldModel);
    //enemyField->setDisabled(true);

}



void MainWindow::showDirections(QModelIndex index)
{
    if(shipsSemaphore<=0) // if no ships left return to avoid exception
        return;

    /** Get selected ship from the list of ships */
    QModelIndexList model_indexes = shipsListWidget->selectionModel()->selectedIndexes(); // list of selected items
    int model_index = model_indexes[0].row(); // get index of selected item (ship)
    QString item = shipsListWidget->currentItem()->text(); // text of selected item (ship)

    int length;
    if(item=="Battleship")
        length = 4;
    else if(item=="Destroyer")
        length = 3;
    else if(item=="Patrol Boat")
        length = 2;

    if(semaphore==0) // starting position of ship selected
    {
        shipsListWidget->setDisabled(true);
        initIndex = index;
        selectDirections(index,length,QColor(245, 190, 190));
        semaphore++;
    }
    else if(semaphore==1) // ending position of ship selected
    {
        int row = index.row();
        int column = index.column();
        if( // ending position is valid
          ( row==initRow    && (column==initLeft || column==initRight) ) ||
          ( column==initCol && (row==initTop     || row==initDown    ) )
          )
        {
            selectDirections(initIndex,length,Qt::white); // remove highlights
            addShip(row,column);
            delete shipsListWidget->item(model_index); // remove selected item (ship)
            semaphore = 0;
            shipsListWidget->setDisabled(false);
            shipsSemaphore--; // to keep track of available ships
        }
    }
}

void MainWindow::selectDirections(QModelIndex index, int length,QColor color)
{
    initRow = index.row();
    initCol = index.column();
    initTop = initRow-length+1;
    initDown = initRow+length-1;
    initLeft = initCol-length+1;
    initRight = initCol+length-1;

    setTableItem(initRow,initCol,"",Qt::blue,initialField,myFieldModel); // center
    #define colNotOverlapping (myFieldModel->item(i,initCol)==0 || myFieldModel->item(i,initCol)->text().isEmpty())
    #define rowNotOverlapping (myFieldModel->item(initRow,j)==0 || myFieldModel->item(initRow,j)->text().isEmpty())
    /* colNotOverlapping and rowNotOverlapping are to avoid overlapping */
    for(int i=initRow-1;i>=initTop  && i>=0   && colNotOverlapping;i--) // top
         setTableItem(i,initCol,"",color,initialField,myFieldModel);
    for(int i=initRow+1;i<=initDown && i<rows && colNotOverlapping;i++) // down
        setTableItem(i,initCol,"",color,initialField,myFieldModel);
    for(int j=initCol-1;j>=initLeft && j>=0   && rowNotOverlapping;j--) // left
        setTableItem(initRow,j,"",color,initialField,myFieldModel);
    for(int j=initCol+1;j<=initRight && j<cols&& rowNotOverlapping;j++) // right
        setTableItem(initRow,j,"",color,initialField,myFieldModel);
}

void MainWindow::setTableItem(int row,int col,QString text, QColor color,QTableView *table,QStandardItemModel *model)
{

    QStandardItem *thisRow = new QStandardItem(text);
    model->setItem(row,col,thisRow);
    model->item(row,col)->setBackground(color);
    table->setModel(model); // re-display table

}

void MainWindow::addShip(int row, int col)
{
    QColor color = Qt::black;
    if(row==initTop) // top
        for(int i=initRow;i>=row;i--)
            setTableItem(i,initCol,"-",color,initialField,myFieldModel);
    else if(row==initDown) // down
        for(int i=initRow;i<=row;i++)
            setTableItem(i,initCol,"-",color,initialField,myFieldModel);
    else if(col==initLeft) // left
        for(int j=initCol;j>=col;j--)
            setTableItem(initRow,j,"-",color,initialField,myFieldModel);
    else if(col==initRight) // right
        for(int j=initCol;j<=col;j++)
            setTableItem(initRow,j,"-",color,initialField,myFieldModel);
}


void MainWindow::on_startButton_clicked()
{
    if(shipsSemaphore>0)
        showMessage(QString::number(shipsSemaphore)+" ships are not arranged yet.");
    else
    {
        stackedWidget->setCurrentWidget(mainPage);

//        myField->setModel(myFieldModel);
    }
}

void MainWindow::showMessage(QString message)
{
    QMessageBox msgBox;
    msgBox.setText(message);
    //msgBox.setInformativeText("");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}
