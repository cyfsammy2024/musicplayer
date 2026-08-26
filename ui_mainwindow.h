/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionImport_File;
    QAction *actionImport_Folder;
    QAction *actionExport_Playlist;
    QAction *actionImport_Playlist;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QTableView *playlistView;
    QTextEdit *lyricsView;
    QProgressBar *progressBar;
    QLabel *currentSongLabel;
    QHBoxLayout *horizontalLayout;
    QLabel *timeLabel;
    QSpacerItem *horizontalSpacer;
    QLabel *volumeLabel;
    QSlider *volumeSlider;
    QLabel *volumePercentageLabel;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *previousButton;
    QPushButton *playButton;
    QPushButton *pauseButton;
    QPushButton *stopButton;
    QPushButton *nextButton;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *playModeButton;
    QPushButton *clearPlaylistButton;
    QPushButton *equalizerButton;
    QMenuBar *menuBar;
    QMenu *menuFile;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1280, 1024);
        actionImport_File = new QAction(MainWindow);
        actionImport_File->setObjectName("actionImport_File");
        actionImport_Folder = new QAction(MainWindow);
        actionImport_Folder->setObjectName("actionImport_Folder");
        actionExport_Playlist = new QAction(MainWindow);
        actionExport_Playlist->setObjectName("actionExport_Playlist");
        actionImport_Playlist = new QAction(MainWindow);
        actionImport_Playlist->setObjectName("actionImport_Playlist");
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName("centralWidget");
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        playlistView = new QTableView(centralWidget);
        playlistView->setObjectName("playlistView");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(playlistView->sizePolicy().hasHeightForWidth());
        playlistView->setSizePolicy(sizePolicy);
        playlistView->setAlternatingRowColors(true);

        horizontalLayout_3->addWidget(playlistView);

        lyricsView = new QTextEdit(centralWidget);
        lyricsView->setObjectName("lyricsView");
        sizePolicy.setHeightForWidth(lyricsView->sizePolicy().hasHeightForWidth());
        lyricsView->setSizePolicy(sizePolicy);
        lyricsView->setMidLineWidth(3);
        lyricsView->setAutoFormatting(QTextEdit::AutoAll);
        lyricsView->setLineWrapMode(QTextEdit::WidgetWidth);
        lyricsView->setReadOnly(true);
        lyricsView->setOverwriteMode(true);
        lyricsView->setTabStopDistance(80.000000000000000);

        horizontalLayout_3->addWidget(lyricsView);


        verticalLayout->addLayout(horizontalLayout_3);

        progressBar = new QProgressBar(centralWidget);
        progressBar->setObjectName("progressBar");
        progressBar->setValue(0);

        verticalLayout->addWidget(progressBar);

        currentSongLabel = new QLabel(centralWidget);
        currentSongLabel->setObjectName("currentSongLabel");

        verticalLayout->addWidget(currentSongLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName("horizontalLayout");
        timeLabel = new QLabel(centralWidget);
        timeLabel->setObjectName("timeLabel");

        horizontalLayout->addWidget(timeLabel);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        volumeLabel = new QLabel(centralWidget);
        volumeLabel->setObjectName("volumeLabel");

        horizontalLayout->addWidget(volumeLabel);

        volumeSlider = new QSlider(centralWidget);
        volumeSlider->setObjectName("volumeSlider");
        volumeSlider->setValue(50);
        volumeSlider->setOrientation(Qt::Horizontal);

        horizontalLayout->addWidget(volumeSlider);

        volumePercentageLabel = new QLabel(centralWidget);
        volumePercentageLabel->setObjectName("volumePercentageLabel");

        horizontalLayout->addWidget(volumePercentageLabel);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        previousButton = new QPushButton(centralWidget);
        previousButton->setObjectName("previousButton");

        horizontalLayout_2->addWidget(previousButton);

        playButton = new QPushButton(centralWidget);
        playButton->setObjectName("playButton");

        horizontalLayout_2->addWidget(playButton);

        pauseButton = new QPushButton(centralWidget);
        pauseButton->setObjectName("pauseButton");
        pauseButton->setEnabled(false);

        horizontalLayout_2->addWidget(pauseButton);

        stopButton = new QPushButton(centralWidget);
        stopButton->setObjectName("stopButton");

        horizontalLayout_2->addWidget(stopButton);

        nextButton = new QPushButton(centralWidget);
        nextButton->setObjectName("nextButton");

        horizontalLayout_2->addWidget(nextButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        playModeButton = new QPushButton(centralWidget);
        playModeButton->setObjectName("playModeButton");

        horizontalLayout_2->addWidget(playModeButton);

        clearPlaylistButton = new QPushButton(centralWidget);
        clearPlaylistButton->setObjectName("clearPlaylistButton");

        horizontalLayout_2->addWidget(clearPlaylistButton);

        equalizerButton = new QPushButton(centralWidget);
        equalizerButton->setObjectName("equalizerButton");

        horizontalLayout_2->addWidget(equalizerButton);


        verticalLayout->addLayout(horizontalLayout_2);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 1280, 31));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName("menuFile");
        MainWindow->setMenuBar(menuBar);

        menuBar->addAction(menuFile->menuAction());
        menuFile->addAction(actionImport_File);
        menuFile->addAction(actionImport_Folder);
        menuFile->addAction(actionExport_Playlist);
        menuFile->addAction(actionImport_Playlist);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\351\237\263\344\271\220\346\222\255\346\224\276\345\231\250", nullptr));
        actionImport_File->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\205\245\346\226\207\344\273\266", nullptr));
        actionImport_Folder->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\205\245\346\226\207\344\273\266\345\244\271", nullptr));
        actionExport_Playlist->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272\346\222\255\346\224\276\345\210\227\350\241\250", nullptr));
        actionImport_Playlist->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\205\245\346\222\255\346\224\276\345\210\227\350\241\250", nullptr));
        lyricsView->setDocumentTitle(QString());
        lyricsView->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'\346\200\235\346\272\220\345\256\213\344\275\223 CN'; font-size:12pt; font-weight:200; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">\346\255\214\350\257\215\345\260\206\346\230\276\347\244\272\345\234\250\350\277\231\351\207\214</p></body></html>", nullptr));
        lyricsView->setPlainText(QCoreApplication::translate("MainWindow", "\346\255\214\350\257\215\345\260\206\346\230\276\347\244\272\345\234\250\350\277\231\351\207\214", nullptr));
        currentSongLabel->setText(QCoreApplication::translate("MainWindow", "\345\275\223\345\211\215\346\222\255\346\224\276: \346\227\240", nullptr));
        timeLabel->setText(QCoreApplication::translate("MainWindow", "0:00 / 0:00", nullptr));
        volumeLabel->setText(QCoreApplication::translate("MainWindow", "\351\237\263\351\207\217:", nullptr));
        volumePercentageLabel->setText(QCoreApplication::translate("MainWindow", "50%", nullptr));
        previousButton->setText(QCoreApplication::translate("MainWindow", "\344\270\212\344\270\200\346\233\262", nullptr));
        playButton->setText(QCoreApplication::translate("MainWindow", "\346\222\255\346\224\276", nullptr));
        pauseButton->setText(QCoreApplication::translate("MainWindow", "\346\232\202\345\201\234", nullptr));
        stopButton->setText(QCoreApplication::translate("MainWindow", "\345\201\234\346\255\242", nullptr));
        nextButton->setText(QCoreApplication::translate("MainWindow", "\344\270\213\344\270\200\346\233\262", nullptr));
        playModeButton->setText(QCoreApplication::translate("MainWindow", "\351\241\272\345\272\217", nullptr));
        clearPlaylistButton->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244\345\210\227\350\241\250", nullptr));
        equalizerButton->setText(QCoreApplication::translate("MainWindow", "\345\235\207\350\241\241\345\231\250", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
