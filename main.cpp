#include "main.h"
#include <QApplication>
#include <QFileDialog>
#include <cmath>
#include <QPainter>
#include <QDebug>

Window:: Window()
{
    setupUi(this);
    QHBoxLayout *layout = new QHBoxLayout(scrollAreaWidgetContents);
    layout->setContentsMargins(0, 0, 0, 0);
    canvas = new Canvas(this);
    layout->addWidget(canvas);
    connectSignals();
}

void
Window:: connectSignals()
{
    connect(actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(actionStart_Scissor, SIGNAL(triggered()), this, SLOT(startScissor()));
    connect(canvas, SIGNAL(mousePressed(int, int)), this, SLOT(onMousePress(int, int)));
    connect(canvas, SIGNAL(mouseMoved(int, int)), this, SLOT(onMouseMove(int, int)));
}

void
Window:: openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open Image", "",
                "Image files (*.jpg *.png );;All files (*.*)" );
    if (!filename.isEmpty())
        openFile(filename);
}

void
Window:: openFile(QString filename)
{
    QImage new_image(filename);
    if (new_image.isNull()) return;

    image = new_image;
    canvas->setImage(image);
    Init_seed = 0;
    Is_start = 0;
    Is_closed = 0;
}

void
Window:: startScissor()
{
    if (canvas->pixmap()->isNull())
        return;
    getGradientMap();
    Is_start = 1;
}

void
Window:: onMousePress(int x, int y)
{
    if(Is_start)
    {
        if(!Init_seed)
        {
            shortPath_vector.clear();
            Init_seed = 1;
        }
        else {
            image = canvas->pixmap()->toImage();    // Save old image with livewire, a temp sol
        }
        seed_x = x;
        seed_y = y;
        qDebug() << "click" << x << y;
    }
}

void
Window:: initVector(int x1, int y1, int x2, int y2)
{
    node_vector.clear();
    int node_num = 0;
    for (int i = y1 ;i <= y2 ; i++) {
        for (int j = x1; j <= x2 ; j++)
        {
            struct Node node;
            node.row = i;   // y
            node.col = j;   // x
            node.state = INITIAL;
            node.totalCost = INF;
            node.prevNode = NULL;
            node.prevNodeNum = 0;
            node.num = node_num;
            int y = (i-1) * 3 + 1;
            int x = (j-1) * 3 + 1;
            int cnt = 0;
            // When pixel is at image boundary, link cost of neighbours is set to INF
            if(i == 0 || j == 0 || i == image.height() - 1 || j ==image.width() - 1)
            {
                for(int k = 0 ;k < 9;k++)
                    node.linkCost[k] = INF;
            }
            else
                for(int l = -1; l <= 1; l++)
                    for(int m = -1; m <= 1;m++)
                    {
                        if(l == 0 && m ==0)
                            node.linkCost[cnt]=0;
                        else
                            node.linkCost[cnt] = qRed(grad_image.pixel(x+l,y+m)); //grad_image.at<Vec3b>(x+l,y+m)[0]
                        cnt++;
                    }

            node_vector.push_back(node);
            node_num++;
        }
    }
    seed_num = (seed_y - y1) * (x2-x1+1) + (seed_x - x1);
    node_vector[seed_num].totalCost = 0;
    node_vector[seed_num].prevNodeNum = seed_num;
    //qDebug() << "seed num" << seed_num << "vector size" << node_num;
}

void
Window:: onMouseMove(int x, int y)
{
    if (Init_seed==0) return; //Return if mouse is not clicked
    //qDebug() << "Point" << x << y;
    int x1 = MIN(x, seed_x);
    int y1 = MIN(y, seed_y);
    int x2 = MAX(x, seed_x);
    int y2 = MAX(y, seed_y);
    //qDebug() << x1 << y1 << x2 << y2;
    //if (x2==x1 or y2==y1) return; // box width and height must not be zero
    initVector(x1,y1,x2,y2);

    priority_queue<Node> que;
    que.push(node_vector[seed_num]);
    while(!que.empty())
    {
        Node q = que.top();                 // This is node with minimum total cost
        que.pop();
        if (node_vector[q.num].state == EXPANDED)
            continue;
        q.state = EXPANDED;
        node_vector[q.num].state = EXPANDED;
        //qDebug() << "q.num" << q.num;
        if (q.col < x1 || q.row < y1 || q.col > x2 || q.row > y2)
            continue;
        //q.row q.col = boundry
        //if(q.row == 0 || q.col == 0 || q.row == image.height() - 1 || q.col == image.width() - 1) // ignore if the pixel is at boundary
        //    continue;
        for(int i = -1 ;i <= 1;i++)         // Iterate x from x-1 to x+1
            for(int j = -1 ;j <= 1;j++)     // Iterate y from y-1 to y+1
            {
                if(i == 0 && j==0) // ignore if the point is q
                    continue;
                // Skip when neighbour is outside bounding box
                if (q.col+j < x1 || q.row+i < y1 || q.col+j > x2 || q.row+i > y2)
                    continue;
                //qDebug() << "neighbour" << neighbour;
                // r is a neighbour of q
                Node r = node_vector[(q.row + i - y1) * (x2-x1+1) + (q.col + j -x1)];
                if(r.state == INITIAL)      // INITIAL means nothing has done yet
                {
                    r.prevNode = &q;
                    r.totalCost = q.totalCost + q.linkCost[(i+1) * 3 + j + 1];
                    r.state = ACTIVE;
                    r.prevNodeNum = q.num;
                    node_vector[r.num].prevNode = &q;
                    node_vector[r.num].prevNodeNum = q.num;
                    node_vector[r.num].totalCost = q.totalCost + q.linkCost[(i+1) * 3 + j + 1];
                    node_vector[r.num].state = ACTIVE;
                    que.push(r);            // Insert r into the que
                }
                else if(r.state == ACTIVE)  // ACTIVE means r is already in que
                {
                    if(q.totalCost + q.linkCost[(i+1) * 3 + j + 1] < r.totalCost)
                    {
                        r.prevNode = &q;
                        r.totalCost = q.totalCost + q.linkCost[(i+1) * 3 + j + 1];
                        r.prevNodeNum = q.num;
                        node_vector[r.num].prevNode = &q;
                        node_vector[r.num].prevNodeNum = q.num;
                        node_vector[r.num].totalCost = q.totalCost + q.linkCost[(i+1) * 3 + j + 1];
                        //update que
                        que.push(r);
                    }
                }
            }
    }
    //qDebug() << "path done";
    Node des_node = node_vector[(y-y1) * (x2-x1+1) + (x-x1)];
    QImage line_image = image.copy();
    QPainter painter(&line_image);
    painter.setPen(Qt::green);
    shortPath_vector.clear();
    shortPath_vector.push_back(QPoint(des_node.col,des_node.row));
    while(des_node.prevNodeNum != seed_num)
    {
        QPoint pointa(des_node.col, des_node.row);
        int next_num = des_node.prevNodeNum;
        des_node = node_vector[next_num];
        QPoint pointb(des_node.col,des_node.row);
        painter.drawLine(pointa, pointb);
        shortPath_vector.push_back(pointb);
    }
    shortPath_vector.push_back(QPoint(seed_x,seed_y));
    painter.end();
    canvas->setImage(line_image);
}

void
Window:: getGradientMap()
{
    QImage tmp_img((image.width() - 2 ) * 3, (image.height() - 2) * 3, QImage::Format_RGB888 );
    tmp_img.fill(Qt::white);
    double maxD = 0;
    double link = 0;
    QImage img = image;
    for(int i = 1 ;i < image.height() - 1; i++)
        for(int j = 1; j < image.width() - 1; j++)
        {
            int y = (i-1) * 3 + 1;
            int x = (j-1) * 3 + 1;

            //x-1,y-1
            double sum = 0;
            sum += pow( qRed(img.pixel(j-1, i)) - qRed(img.pixel(j, i-1)), 2);
            sum += pow( qGreen(img.pixel(j-1, i)) - qGreen(img.pixel(j, i-1)), 2);
            sum += pow( qBlue(img.pixel(j-1, i)) - qBlue(img.pixel(j, i-1)), 2);
            link = sqrt(sum / 6.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x-1, y-1, qRgb(link, link, link));

            //x-1,y+1
            sum = 0;
            sum += pow( qRed(img.pixel(j-1, i)) - qRed(img.pixel(j, i+1)), 2);
            sum += pow( qGreen(img.pixel(j-1, i)) - qGreen(img.pixel(j, i+1)), 2);
            sum += pow( qBlue(img.pixel(j-1, i)) - qBlue(img.pixel(j, i+1)), 2);

            link = sqrt(sum / 6.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x-1, y+1, qRgb(link, link, link));

            //x+1,y-1
            sum = 0;
            sum += pow( qRed(img.pixel(j, i-1)) - qRed(img.pixel(j+1, i)), 2);
            sum += pow( qGreen(img.pixel(j, i-1)) - qGreen(img.pixel(j+1, i)), 2);
            sum += pow( qBlue(img.pixel(j, i-1)) - qBlue(img.pixel(j+1, i)), 2);

            link = sqrt(sum / 6.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x+1, y-1, qRgb(link, link, link));

            //x+1,y+1
            sum = 0;
            sum += pow( qRed(img.pixel(j+1, i)) - qRed(img.pixel(j, i+1)), 2);
            sum += pow( qGreen(img.pixel(j+1, i)) - qGreen(img.pixel(j, i+1)), 2);
            sum += pow( qBlue(img.pixel(j+1, i)) - qBlue(img.pixel(j, i+1)), 2);

            link = sqrt(sum / 6.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x+1, y+1, qRgb(link, link, link));

            //x-1,y
            sum = 0;
            sum += pow( (qRed(img.pixel(j, i-1)) + qRed(img.pixel(j-1, i-1)) - qRed(img.pixel(j, i+1)) - qRed(img.pixel(j-1, i+1)))/4.0, 2);
            sum += pow( (qGreen(img.pixel(j, i-1)) + qGreen(img.pixel(j-1, i-1)) - qGreen(img.pixel(j, i+1)) - qGreen(img.pixel(j-1, i+1)))/4.0, 2);
            sum += pow( (qBlue(img.pixel(j, i-1)) + qBlue(img.pixel(j-1, i-1)) - qBlue(img.pixel(j, i+1)) - qBlue(img.pixel(j-1, i+1)))/4.0, 2);

            link = sqrt(sum / 3.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x-1, y, qRgb(link, link, link));

            //x,y-1
            sum = 0;
            sum += pow( (qRed(img.pixel(j-1, i)) + qRed(img.pixel(j-1, i-1)) - qRed(img.pixel(j+1, i-1)) - qRed(img.pixel(j+1, i)))/4.0, 2);
            sum += pow( (qGreen(img.pixel(j-1, i)) + qGreen(img.pixel(j-1, i-1)) - qGreen(img.pixel(j+1, i-1)) - qGreen(img.pixel(j+1, i)))/4.0, 2);
            sum += pow( (qBlue(img.pixel(j-1, i)) + qBlue(img.pixel(j-1, i-1)) - qBlue(img.pixel(j+1, i-1)) - qBlue(img.pixel(j+1, i)))/4.0, 2);

            link = sqrt(sum / 3.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x, y-1, qRgb(link, link, link));

            //x,y+1
            sum = 0;
            sum += pow( (qRed(img.pixel(j+1, i)) + qRed(img.pixel(j+1, i+1)) - qRed(img.pixel(j-1, i)) - qRed(img.pixel(j-1, i+1)) )/4.0, 2);
            sum += pow( (qGreen(img.pixel(j+1, i)) + qGreen(img.pixel(j+1, i+1)) - qGreen(img.pixel(j-1, i)) - qGreen(img.pixel(j-1, i+1)) )/4.0, 2);
            sum += pow( (qBlue(img.pixel(j+1, i)) + qBlue(img.pixel(j+1, i+1)) - qBlue(img.pixel(j-1, i)) - qBlue(img.pixel(j-1, i+1)) )/4.0, 2);

            link = sqrt(sum / 3.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x, y+1, qRgb(link, link, link));

            //x+1,y
            sum = 0;
            sum += pow( (qRed(img.pixel(j, i+1)) + qRed(img.pixel(j+1, i+1)) - qRed(img.pixel(j, i-1)) - qRed(img.pixel(j+1, i-1)) )/4.0, 2);
            sum += pow( (qGreen(img.pixel(j, i+1)) + qGreen(img.pixel(j+1, i+1)) - qGreen(img.pixel(j, i-1)) - qGreen(img.pixel(j+1, i-1)) )/4.0, 2);
            sum += pow( (qBlue(img.pixel(j, i+1)) + qBlue(img.pixel(j+1, i+1)) - qBlue(img.pixel(j, i-1)) - qBlue(img.pixel(j+1, i-1)) )/4.0, 2);

            link = sqrt(sum / 3.0);
            if (link > maxD)
                maxD = link;
            tmp_img.setPixel(x+1, y, qRgb(link, link, link));
        }

    qDebug() << "gradient maxD =" << maxD;

    //update cost
    //double a = 1.0;
    for(int i = 1 ;i < image.height() - 1; i++)
        for(int j = 1; j < image.width() - 1; j++)
        {
            int y = (i-1) * 3 + 1;
            int x = (j-1) * 3 + 1;
            //tmp_img.setPixel(x,y, qRgb(255, 255, 255));
            int clr;

            clr = (maxD - qRed(tmp_img.pixel(x-1,y-1))) * 1.414;
            tmp_img.setPixel(x-1,y-1, qRgb(clr,clr,clr));

            clr = (maxD - qGreen(tmp_img.pixel(x+1,y-1))) * 1.414;
            tmp_img.setPixel(x+1,y-1, qRgb(clr, clr, clr));

            clr = (maxD - qGreen(tmp_img.pixel(x-1,y+1))) * 1.414;
            tmp_img.setPixel(x-1,y+1, qRgb(clr,clr,clr));

            clr = (maxD - qGreen(tmp_img.pixel(x+1,y+1))) * 1.414;
            tmp_img.setPixel(x+1,y+1, qRgb(clr,clr,clr));

            clr = (maxD - qGreen(tmp_img.pixel(x,y-1)));
            tmp_img.setPixel(x,y-1, qRgb(clr,clr,clr));

            clr = (maxD - qGreen(tmp_img.pixel(x-1,y)));
            tmp_img.setPixel(x-1,y, qRgb(clr,clr,clr));

            clr = (maxD - qGreen(tmp_img.pixel(x+1,y)));
            tmp_img.setPixel(x+1,y, qRgb(clr,clr,clr));

            clr = (maxD - qGreen(tmp_img.pixel(x,y+1)));
            tmp_img.setPixel(x,y+1, qRgb(clr,clr,clr));
        }
    grad_image = tmp_img;
    qDebug() << "grad image created";
}



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    //app.setOrganizationName("QmageView");
    //app.setApplicationName("QmageView");
    Window *win = new Window();
    win->openFile("/home/subha/Arindam.jpg");
    win->show();
    return app.exec();
}
