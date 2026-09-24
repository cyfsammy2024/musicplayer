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
    QAction *actionEqualizer;
    QAction *actionLyrics;
    QAction *actionEditTags;
    QAction *actionCheck_Update;
    QAction *actionAbout;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QTableView *playlistView;
    QProgressBar *progressBar;
    QHBoxLayout *horizontalLayout;
    QLabel *currentSongLabel;
    QSpacerItem *horizontalSpacer;
    QLabel *timeLabel;
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
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuPlugins;
    QMenu *menuAbout;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(652, 503);
        actionImport_File = new QAction(MainWindow);
        actionImport_File->setObjectName("actionImport_File");
        actionImport_Folder = new QAction(MainWindow);
        actionImport_Folder->setObjectName("actionImport_Folder");
        actionExport_Playlist = new QAction(MainWindow);
        actionExport_Playlist->setObjectName("actionExport_Playlist");
        actionImport_Playlist = new QAction(MainWindow);
        actionImport_Playlist->setObjectName("actionImport_Playlist");
        actionEqualizer = new QAction(MainWindow);
        actionEqualizer->setObjectName("actionEqualizer");
        actionLyrics = new QAction(MainWindow);
        actionLyrics->setObjectName("actionLyrics");
        actionEditTags = new QAction(MainWindow);
        actionEditTags->setObjectName("actionEditTags");
        actionCheck_Update = new QAction(MainWindow);
        actionCheck_Update->setObjectName("actionCheck_Update");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
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


        verticalLayout->addLayout(horizontalLayout_3);

        progressBar = new QProgressBar(centralWidget);
        progressBar->setObjectName("progressBar");
        progressBar->setValue(0);

        verticalLayout->addWidget(progressBar);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName("horizontalLayout");
        currentSongLabel = new QLabel(centralWidget);
        currentSongLabel->setObjectName("currentSongLabel");

        horizontalLayout->addWidget(currentSongLabel);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        timeLabel = new QLabel(centralWidget);
        timeLabel->setObjectName("timeLabel");

        horizontalLayout->addWidget(timeLabel);

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


        verticalLayout->addLayout(horizontalLayout_2);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 652, 28));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName("menuFile");
        menuPlugins = new QMenu(menuBar);
        menuPlugins->setObjectName("menuPlugins");
        menuAbout = new QMenu(menuBar);
        menuAbout->setObjectName("menuAbout");
        MainWindow->setMenuBar(menuBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuPlugins->menuAction());
        menuBar->addAction(menuAbout->menuAction());
        menuFile->addAction(actionImport_File);
        menuFile->addAction(actionImport_Folder);
        menuFile->addAction(actionExport_Playlist);
        menuFile->addAction(actionImport_Playlist);
        menuPlugins->addAction(actionEqualizer);
        menuPlugins->addAction(actionLyrics);
        menuPlugins->addAction(actionEditTags);
        menuAbout->addAction(actionCheck_Update);
        menuAbout->addAction(actionAbout);

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
        actionEqualizer->setText(QCoreApplication::translate("MainWindow", "\345\235\207\350\241\241\345\231\250", nullptr));
        actionLyrics->setText(QCoreApplication::translate("MainWindow", "\346\255\214\350\257\215", nullptr));
        actionEditTags->setText(QCoreApplication::translate("MainWindow", "\346\240\207\347\255\276\347\274\226\350\276\221", nullptr));
        actionCheck_Update->setText(QCoreApplication::translate("MainWindow", "\346\243\200\346\237\245\346\233\264\346\226\260", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
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
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        menuPlugins->setTitle(QCoreApplication::translate("MainWindow", "\346\217\222\344\273\266", nullptr));
        menuAbout->setTitle(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
