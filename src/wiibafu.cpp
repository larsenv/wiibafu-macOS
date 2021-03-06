/***************************************************************************
 *   Copyright (C) 2010-2013 Kai Heitkamp                                  *
 *   dynup@ymail.com | http://dynup.de.vu                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "wiibafu.h"
#include "ui_wiibafu.h"

WiiBaFu::WiiBaFu(QWidget *parent) : QMainWindow(parent), ui(new Ui::WiiBaFu) {
    wiTools = new WiTools(this);
    common = new Common(this);
    settings = new Settings(this);
    wiibafudialog = new WiiBaFuDialog(this);
    coverViewDialog = new CoverViewDialog(this);
    wbfsDialog = new WBFSDialog(this);

    timer = new QTimer(this);

    filesListModel = new QStandardItemModel(this);
    dvdListModel = new QStandardItemModel(this);
    wbfsListModel = new QStandardItemModel(this);

    ui->setupUi(this);
    setWindowTitle("Wii Backup Fusion " + QCoreApplication::applicationVersion());
    setupMainProgressBar();
    setupConnections();
    setupToolsButton();
    setupContextMenus();
    setupGeometry();

    setGameListAttributes(ui->filesTab_tableView);
    setGameListAttributes(ui->dvdTab_tableView);
    setGameListAttributes(ui->wbfsTab_tableView);

    #ifdef Q_OS_MACX
        setMacOSXStyle();
    #endif

    if (!WIIBAFU_SETTINGS.value("Main/LogFile", QVariant("")).toString().isEmpty()) {
        logFile.setFileName(WIIBAFU_SETTINGS.value("Main/LogFile", QVariant("")).toString());
        logFile.open(QIODevice::WriteOnly);
    }

    lastOutputPath = WIIBAFU_SETTINGS.value("Main/LastOutputPath", QDir::homePath()).toString();
    logFindFirstTime = true;

    setStatusBarText(tr("Ready."));

    addEntryToLog(tr("(%1) Wii Backup Fusion %2 started.").arg(QDateTime::currentDateTime().toString(), QCoreApplication::applicationVersion()), WiTools::Info);
    addEntryToLog(wiTools->witVersion(), WiTools::Info);
    addEntryToLog(wiTools->wwtVersion(), WiTools::Info);

    QString witPath = wiTools->witTitlesPath();
    if (witPath.contains("witTitles:")) {
        setStatusBarText(tr("Titles not found!"));
        addEntryToLog(tr("Titles not found!\n"), WiTools::Info);
    }
    else {
        addEntryToLog(QDir::toNativeSeparators(tr("Titles found in: %1\n").arg(witPath)), WiTools::Info);
    }
}

void WiiBaFu::setupConnections() {
    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");
    qRegisterMetaType<WiTools::LogType>("WiTools::LogType");
    qRegisterMetaType<WiTools::WitStatus>("WiTools::WitStatus");

    connect(this, SIGNAL(cancelTransfer()), wiTools, SLOT(cancelTransfer()));
    connect(this, SIGNAL(cancelLoading()), wiTools, SLOT(cancelLoading()));
    connect(this, SIGNAL(cancelVerifying()), wiTools, SLOT(cancelVerifying()));

    connect(this, SIGNAL(startBusy()), this, SLOT(startMainProgressBarBusy()));
    connect(this, SIGNAL(stopBusy()), this, SLOT(stopMainProgressBarBusy()));
    connect(timer, SIGNAL(timeout()), this, SLOT(showMainProgressBarBusy()));

    connect(wiTools, SIGNAL(setMainProgressBar(int, QString)), this, SLOT(setMainProgressBar(int,QString)));
    connect(wiTools, SIGNAL(setMainProgressBarVisible(bool)), this, SLOT(setMainProgressBarVisible(bool)));

    connect(wiTools, SIGNAL(setInfoTextWBFS(QString)), this, SLOT(setWBFSInfoText(QString)));
    connect(wiTools, SIGNAL(setProgressBarWBFS(int, int, int, QString)), this, SLOT(setWBFSProgressBar(int, int, int, QString)));
    connect(wiTools, SIGNAL(showStatusBarMessage(QString)), this, SLOT(setStatusBarText(QString)));
    connect(wiTools, SIGNAL(newLogEntry(QString, WiTools::LogType)), this, SLOT(addEntryToLog(QString, WiTools::LogType)));
    connect(wiTools, SIGNAL(newLogEntries(QStringList,WiTools::LogType)), this, SLOT(addEntriesToLog(QStringList,WiTools::LogType)));
    connect(wiTools, SIGNAL(newWitCommandLineLogEntry(QString, QStringList)), this, SLOT(addWitCommandLineToLog(QString, QStringList)));

    connect(wiTools, SIGNAL(newFilesGameListModel()), this, SLOT(setFilesGameListModel()));
    connect(wiTools, SIGNAL(loadingGamesCanceled()), this, SLOT(loadingGamesCanceled()));
    connect(wiTools, SIGNAL(loadingGamesFailed(WiTools::WitStatus)), this, SLOT(loadingGamesFailed(WiTools::WitStatus)));
    connect(wiTools, SIGNAL(newDVDGameListModel()), this, SLOT(setDVDGameListModel()));
    connect(wiTools, SIGNAL(newWBFSGameListModel()), this, SLOT(setWBFSGameListModel()));

    connect(wiTools, SIGNAL(transferFilesToWBFS_finished(WiTools::WitStatus)), this, SLOT(transferFilesToWBFS_finished(WiTools::WitStatus)));
    connect(wiTools, SIGNAL(transferFilesToImage_finished(WiTools::WitStatus)), this, SLOT(transferFilesToImage_finished(WiTools::WitStatus)));
    connect(wiTools, SIGNAL(extractImage_finished(WiTools::WitStatus)), this, SLOT(extractImage_finished(WiTools::WitStatus)));

    connect(wiTools, SIGNAL(transferDVDToWBFS_finished(WiTools::WitStatus)), this, SLOT(transferDVDToWBFS_finished(WiTools::WitStatus)));
    connect(wiTools, SIGNAL(transferDVDToImage_finished(WiTools::WitStatus)), this, SLOT(transferDVDToImage_finished(WiTools::WitStatus)));
    connect(wiTools, SIGNAL(extractDVD_finished(WiTools::WitStatus)), this, SLOT(extractDVD_finished(WiTools::WitStatus)));

    connect(wiTools, SIGNAL(transferWBFSToImage_finished(WiTools::WitStatus)), this, SLOT(transferWBFSToImage_finished(WiTools::WitStatus)));
    connect(wiTools, SIGNAL(extractWBFS_finished(WiTools::WitStatus)), this, SLOT(extractWBFS_finished(WiTools::WitStatus)));

    connect(wiTools, SIGNAL(removeGamesFromWBFS_successfully()), this, SLOT(on_wbfsTab_pushButton_Load_clicked()));

    connect(wiTools, SIGNAL(verifyGame_finished(WiTools::WitStatus)), this, SLOT(verifyGame_finished(WiTools::WitStatus)));

    connect(wiTools, SIGNAL(patchGameImage_finished(WiTools::WitStatus)), this, SLOT(filesGame_Patch_finished(WiTools::WitStatus)));

    connect(wiTools, SIGNAL(stopBusy()), this, SLOT(stopMainProgressBarBusy()));

    connect(common, SIGNAL(newGame3DCover(QImage)), this, SLOT(showGame3DCover(QImage)));
    connect(common, SIGNAL(newGameFullHQCover(QImage)), this, SLOT(showGameFullHQCover(QImage)));
    connect(common, SIGNAL(newGameDiscCover(QImage)), this, SLOT(showGameDiscCover(QImage)));

    connect(common, SIGNAL(showStatusBarMessage(QString)), this, SLOT(setStatusBarText(QString)));
    connect(common, SIGNAL(newLogEntry(QString, WiTools::LogType)), this, SLOT(addEntryToLog(QString, WiTools::LogType)));

    connect(common, SIGNAL(setMainProgressBarVisible(bool)), this, SLOT(setMainProgressBarVisible(bool)));
    connect(common, SIGNAL(setMainProgressBar(int,QString)), this, SLOT(setMainProgressBar(int,QString)));
}

void WiiBaFu::setupToolsButton() {
    QMenu *toolsContextMenu = new QMenu(this);

    QAction *action_CheckWBFS = new QAction(QIcon(":/images/check.png"), tr("&Check WBFS"), this);
    QAction *action_DumpWBFS = new QAction(QIcon(":/images/dump-wbfs.png"), tr("&Dump WBFS"), this);
    QAction *action_CreateWBFS = new QAction(QIcon(":/images/create-wbfs.png"), tr("C&reate WBFS"), this);

    action_VerifyGame = new QAction(QIcon(":/images/verify-game.png"), tr("&Verify Game"), this);
    QAction *action_Compare = new QAction(QIcon(":/images/compare.png"), tr("Com&pare Files/WBFS"), this);

    QAction *action_UpdateTitles = new QAction(QIcon(":/images/update.png"), tr("&Update titles"), this);

    QAction *action_Seperator1 = new QAction(this);
    QAction *action_Seperator2 = new QAction(this);
    action_Seperator1->setSeparator(true);
    action_Seperator2->setSeparator(true);

    action_CheckWBFS->setIconVisibleInMenu(true);
    action_DumpWBFS->setIconVisibleInMenu(true);
    action_CreateWBFS->setIconVisibleInMenu(true);
    action_VerifyGame->setIconVisibleInMenu(true);
    action_Compare->setIconVisibleInMenu(true);
    action_UpdateTitles->setIconVisibleInMenu(true);

    connect(action_CheckWBFS, SIGNAL(triggered()), this, SLOT(tools_CheckWBFS_triggered()));
    connect(action_DumpWBFS, SIGNAL(triggered()), this, SLOT(tools_DumpWBFS_triggered()));
    connect(action_CreateWBFS, SIGNAL(triggered()), this, SLOT(tools_CreateWBFS_triggered()));
    connect(action_VerifyGame, SIGNAL(triggered()), this, SLOT(tools_VerifyGame_triggered()));
    connect(action_Compare, SIGNAL(triggered()), this, SLOT(tools_Compare_triggered()));
    connect(action_UpdateTitles, SIGNAL(triggered()), this, SLOT(tools_UpdateTitles_triggered()));

    toolsContextMenu->addAction(action_CheckWBFS);
    toolsContextMenu->addAction(action_DumpWBFS);
    toolsContextMenu->addAction(action_CreateWBFS);
    toolsContextMenu->addAction(action_Seperator1);
    toolsContextMenu->addAction(action_VerifyGame);
    toolsContextMenu->addAction(action_Compare);
    toolsContextMenu->addAction(action_Seperator2);
    toolsContextMenu->addAction(action_UpdateTitles);

    QToolButton* toolsButton = new QToolButton(this);
    toolsButton->setIcon(QIcon(":/images/tools.png"));
    toolsButton->setText(tr("Tools"));
    toolsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolsButton->setMenu(toolsContextMenu);
    toolsButton->setPopupMode(QToolButton::InstantPopup);

    ui->toolBar->insertWidget(ui->toolBar->actions().at(7), toolsButton);
}

void WiiBaFu::setupContextMenus() {
    QAction *action_filesGame_TransferToWBFS = new QAction(QIcon(":/images/transfer-to-wbfs.png"), tr("Tranfer to &WBFS"), this);
    QAction *action_filesGame_TransferToImage = new QAction(QIcon(":/images/transfer-to-image.png"), tr("Transfer to &image"), this);
    QAction *action_filesGame_ExtractImage = new QAction(QIcon(":/images/extract-image.png"), tr("E&xtract image"), this);
    action_filesGame_TransferToWBFS_Patch = new QAction(QIcon(":/images/transfer-to-wbfs-patch.png"), tr("Tranfer to WBFS with patching"), this);
    action_filesGame_TransferToImage_Patch = new QAction(QIcon(":/images/transfer-to-image-patch.png"), tr("Transfer to image with patching"), this);
    action_filesGame_ExtractImage_Patch = new QAction(QIcon(":/images/extract-image-patch.png"), tr("Extract image with patching"), this);
    QAction *action_filesGame_Verify = new QAction(QIcon(":/images/verify-game.png"), tr("&Verify game"), this);
    action_filesGame_Patch = new QAction(QIcon(":/images/patch.png"), tr("&Patch"), this);
    QAction *action_filesGame_ShowInfo = new QAction(QIcon(":/images/info.png"), tr("Show i&nfo"), this);
    QAction *action_filesGame_Seperator1 = new QAction(this);
    QAction *action_filesGame_Seperator2 = new QAction(this);
    QAction *action_filesGame_Seperator3 = new QAction(this);
    QAction *action_filesGame_Seperator4 = new QAction(this);
    action_filesGame_Seperator1->setSeparator(true);
    action_filesGame_Seperator2->setSeparator(true);
    action_filesGame_Seperator3->setSeparator(true);
    action_filesGame_Seperator4->setSeparator(true);

    QAction *action_wbfsGame_Transfer = new QAction(QIcon(":/images/transfer-to-image.png"), tr("&Tranfer"), this);
    QAction *action_wbfsGame_Extract = new QAction(QIcon(":/images/hdd-extract.png"), tr("E&xtract"), this);
    action_wbfsGame_Transfer_Patch = new QAction(QIcon(":/images/transfer-to-image-patch.png"), tr("Tranfer with patching"), this);
    action_wbfsGame_Extract_Patch = new QAction(QIcon(":/images/hdd-extract-patch.png"), tr("Extract with patching"), this);
    QAction *action_wbfsGame_Remove = new QAction(QIcon(":/images/delete.png"), tr("&Remove"), this);
    QAction *action_wbfsGame_Verify = new QAction(QIcon(":/images/verify-game.png"), tr("&Verify"), this);
    QAction *action_wbfsGame_ShowInfo = new QAction(QIcon(":/images/info.png"), tr("Show i&nfo"), this);
    QAction *action_wbfsGame_Seperator1 = new QAction(this);
    QAction *action_wbfsGame_Seperator2 = new QAction(this);
    QAction *action_wbfsGame_Seperator3 = new QAction(this);
    action_wbfsGame_Seperator1->setSeparator(true);
    action_wbfsGame_Seperator2->setSeparator(true);
    action_wbfsGame_Seperator3->setSeparator(true);

    action_filesGame_TransferToWBFS->setIconVisibleInMenu(true);
    action_filesGame_TransferToImage->setIconVisibleInMenu(true);
    action_filesGame_ExtractImage->setIconVisibleInMenu(true);
    action_filesGame_TransferToWBFS_Patch->setIconVisibleInMenu(true);
    action_filesGame_TransferToImage_Patch->setIconVisibleInMenu(true);
    action_filesGame_ExtractImage_Patch->setIconVisibleInMenu(true);
    action_filesGame_Verify->setIconVisibleInMenu(true);
    action_filesGame_Patch->setIconVisibleInMenu(true);
    action_filesGame_ShowInfo->setIconVisibleInMenu(true);

    action_wbfsGame_Transfer->setIconVisibleInMenu(true);
    action_wbfsGame_Extract->setIconVisibleInMenu(true);
    action_wbfsGame_Transfer_Patch->setIconVisibleInMenu(true);
    action_wbfsGame_Extract_Patch->setIconVisibleInMenu(true);
    action_wbfsGame_Remove->setIconVisibleInMenu(true);
    action_wbfsGame_Verify->setIconVisibleInMenu(true);
    action_wbfsGame_ShowInfo->setIconVisibleInMenu(true);

    connect(action_filesGame_TransferToWBFS, SIGNAL(triggered()), this, SLOT(on_filesTab_pushButton_TransferToWBFS_clicked()));
    connect(action_filesGame_TransferToImage, SIGNAL(triggered()), this, SLOT(on_filesTab_pushButton_TransferToImage_clicked()));
    connect(action_filesGame_ExtractImage, SIGNAL(triggered()), this, SLOT(on_filesTab_pushButton_ExtractImage_clicked()));
    connect(action_filesGame_TransferToWBFS_Patch, SIGNAL(triggered()), this, SLOT(filesTab_ContextMenu_TransferToWBFS_Patch()));
    connect(action_filesGame_TransferToImage_Patch, SIGNAL(triggered()), this, SLOT(filesTab_ContextMenu_TransferToImage_Patch()));
    connect(action_filesGame_ExtractImage_Patch, SIGNAL(triggered()), this, SLOT(filesTab_ContextMenu_ExtractImage_Path()));
    connect(action_filesGame_Verify, SIGNAL(triggered()), this, SLOT(tools_VerifyGame_triggered()));
    connect(action_filesGame_Patch, SIGNAL(triggered()), this, SLOT(filesGame_Patch()));
    connect(action_filesGame_ShowInfo, SIGNAL(triggered()), this, SLOT(on_filesTab_pushButton_ShowInfo_clicked()));

    connect(action_wbfsGame_Transfer, SIGNAL(triggered()), this, SLOT(on_wbfsTab_pushButton_Transfer_clicked()));
    connect(action_wbfsGame_Extract, SIGNAL(triggered()), this, SLOT(on_wbfsTab_pushButton_Extract_clicked()));
    connect(action_wbfsGame_Transfer_Patch, SIGNAL(triggered()), this, SLOT(wbfsTab_ContextMenu_Transfer_Patch()));
    connect(action_wbfsGame_Extract_Patch, SIGNAL(triggered()), this, SLOT(wbfsTab_ContextMenu_Extract_Path()));
    connect(action_wbfsGame_Verify, SIGNAL(triggered()), this, SLOT(tools_VerifyGame_triggered()));
    connect(action_wbfsGame_Remove, SIGNAL(triggered()), this, SLOT(on_wbfsTab_pushButton_Remove_clicked()));
    connect(action_wbfsGame_ShowInfo, SIGNAL(triggered()), this, SLOT(on_wbfsTab_pushButton_ShowInfo_clicked()));

    ui->filesTab_tableView->addAction(action_filesGame_TransferToWBFS);
    ui->filesTab_tableView->addAction(action_filesGame_TransferToImage);
    ui->filesTab_tableView->addAction(action_filesGame_ExtractImage);
    ui->filesTab_tableView->addAction(action_filesGame_Seperator1);
    ui->filesTab_tableView->addAction(action_filesGame_TransferToWBFS_Patch);
    ui->filesTab_tableView->addAction(action_filesGame_TransferToImage_Patch);
    ui->filesTab_tableView->addAction(action_filesGame_ExtractImage_Patch);
    ui->filesTab_tableView->addAction(action_filesGame_Seperator2);
    ui->filesTab_tableView->addAction(action_filesGame_Verify);
    ui->filesTab_tableView->addAction(action_filesGame_Seperator3);
    ui->filesTab_tableView->addAction(action_filesGame_Patch);
    ui->filesTab_tableView->addAction(action_filesGame_Seperator4);
    ui->filesTab_tableView->addAction(action_filesGame_ShowInfo);

    ui->wbfsTab_tableView->addAction(action_wbfsGame_Transfer);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Extract);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Seperator1);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Transfer_Patch);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Extract_Patch);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Seperator2);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Verify);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Remove);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_Seperator3);
    ui->wbfsTab_tableView->addAction(action_wbfsGame_ShowInfo);

    ui->filesTab_tableView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->wbfsTab_tableView->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void WiiBaFu::setupMainProgressBar() {
    progressBar_Main = new QProgressBar(this);
    progressBar_Main->setObjectName("progressBarMain");
    progressBar_Main->setVisible(false);
    progressBar_Main->setMinimum(0);
    progressBar_Main->setMaximum(100);
    progressBar_Main->setValue(0);

    progressBar_Main->setMaximumHeight(16);
    ui->statusBar->addPermanentWidget(progressBar_Main);

    #ifdef Q_OS_MACX
        ui->wbfsTab_label_Info->setVisible(true);
    #else
        ui->wbfsTab_label_Info->setVisible(false);
    #endif
}

void WiiBaFu::setupGeometry() {
    if (WIIBAFU_SETTINGS.contains("MainWindow/x")) {
        QRect rect;
        rect.setX(WIIBAFU_SETTINGS.value("MainWindow/x", QVariant(0)).toInt());
        rect.setY(WIIBAFU_SETTINGS.value("MainWindow/y", QVariant(0)).toInt());
        rect.setWidth(WIIBAFU_SETTINGS.value("MainWindow/width", QVariant(800)).toInt());
        rect.setHeight(WIIBAFU_SETTINGS.value("MainWindow/height", QVariant(600)).toInt());

        this->setGeometry(rect);
    }
}

void WiiBaFu::setMacOSXStyle() {
    if (WIIBAFU_SETTINGS.value("Main/MacOSXStyle", QVariant("Aqua")).toString().contains("BrushedMetal")) {
        ui->filesTab_frame_Buttons->setFrameShape(QFrame::NoFrame);
        ui->dvdTab_frame_Info->setFrameShape(QFrame::NoFrame);
        ui->dvdTab_frame_Buttons->setFrameShape(QFrame::NoFrame);
        ui->wbfsTab_frame_Buttons->setFrameShape(QFrame::NoFrame);
        ui->wbfsTab_frame_Info->setFrameShape(QFrame::NoFrame);
        ui->infoTab_frame_Infos->setFrameShape(QFrame::NoFrame);
        ui->infoTab_frame_Buttons->setFrameShape(QFrame::NoFrame);
        ui->logTab_frame_Buttons->setFrameShape(QFrame::NoFrame);

        this->setAttribute(Qt::WA_MacBrushedMetal, true);
    }
    else {
        ui->filesTab_frame_Buttons->setFrameShape(QFrame::Box);
        ui->dvdTab_frame_Info->setFrameShape(QFrame::StyledPanel);
        ui->dvdTab_frame_Buttons->setFrameShape(QFrame::Box);
        ui->wbfsTab_frame_Buttons->setFrameShape(QFrame::Box);
        ui->wbfsTab_frame_Info->setFrameShape(QFrame::StyledPanel);
        ui->infoTab_frame_Infos->setFrameShape(QFrame::StyledPanel);
        ui->infoTab_frame_Buttons->setFrameShape(QFrame::Box);
        ui->logTab_frame_Buttons->setFrameShape(QFrame::Box);

        this->setAttribute(Qt::WA_MacBrushedMetal, false);
    }
}

void WiiBaFu::setGameListAttributes(QTableView *gameTableView) {
    gameTableView->setShowGrid(WIIBAFU_SETTINGS.value("GameListBehavior/ShowGrid", QVariant(false)).toBool());
    gameTableView->setAlternatingRowColors(WIIBAFU_SETTINGS.value("GameListBehavior/AlternatingRowColors", QVariant(true)).toBool());
    gameTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    gameTableView->setHorizontalScrollMode((QHeaderView::ScrollMode)WIIBAFU_SETTINGS.value("GameListBehavior/ScrollMode", QVariant(QHeaderView::ScrollPerPixel)).toInt());
    gameTableView->setVerticalScrollMode((QHeaderView::ScrollMode)WIIBAFU_SETTINGS.value("GameListBehavior/ScrollMode", QVariant(QHeaderView::ScrollPerPixel)).toInt());

    if (gameTableView != ui->dvdTab_tableView) {
        gameTableView->verticalHeader()->hide();
        gameTableView->horizontalHeader()->setSectionsMovable(true);
        gameTableView->verticalHeader()->setDefaultSectionSize(20);
        gameTableView->horizontalHeader()->setSectionResizeMode((QHeaderView::ResizeMode)WIIBAFU_SETTINGS.value("GameListBehavior/ResizeMode", QVariant(QHeaderView::ResizeToContents)).toInt());
        gameTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        gameTableView->setSelectionMode((QAbstractItemView::SelectionMode)WIIBAFU_SETTINGS.value("GameListBehavior/SelectionMode", QVariant(QAbstractItemView::ExtendedSelection)).toInt());
        gameTableView->setSortingEnabled(true);
        gameTableView->setTabKeyNavigation(false);
    }
    else {
        gameTableView->horizontalHeader()->hide();
        gameTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        gameTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        gameTableView->setSelectionMode(QAbstractItemView::NoSelection);
    }

    gameTableView->setLocale(locale());
}

void WiiBaFu::action_Files_triggered() {
    setView(0);
}

void WiiBaFu::action_DVD_triggered() {
    setView(1);
}

void WiiBaFu::action_WBFS_triggered() {
    setView(2);
}

void WiiBaFu::action_Info_triggered() {
    setView(3);
}

void WiiBaFu::action_Log_triggered() {
    setView(4);
}

void WiiBaFu::action_Reset_triggered() {
    const int index = ui->stackedWidget->currentIndex();

    switch (index) {
        case 0:
            filesListModel->clear();
        case 1:
            dvdListModel->clear();
            ui->dvdTab_label_DiscCover->clear();
            ui->toolBar->actions().at(1)->setText(tr("DVD"));
        case 2:
            wbfsListModel->clear();
            ui->wbfsTab_label_Info->clear();
            ui->wbfsTab_progressBar->setValue(0);
        case 3:
            on_infoTab_pushButton_Reset_clicked();
        case 4:
            on_logTab_pushButton_Clear_clicked();
    }

    setStatusBarText(tr("Ready."));
}

void WiiBaFu::action_Settings_triggered() {
    options_Settings_triggered();
}

void WiiBaFu::action_About_triggered() {
    help_About_triggered();
}

void WiiBaFu::action_Exit_triggered() {
    close();
}

void::WiiBaFu::setView(const int index) {
    ui->stackedWidget->setCurrentIndex(index);
}

void WiiBaFu::options_Settings_triggered() {
    int language = WIIBAFU_SETTINGS.value("Main/GameLanguage", QVariant(0)).toInt();

    #ifdef Q_OS_MACX
        QString macOSXStyle = WIIBAFU_SETTINGS.value("Main/MacOSXStyle", QVariant("Aqua")).toString();
    #endif

    if (settings->exec() == QDialog::Accepted) {
        setGameListAttributes(ui->filesTab_tableView);
        setGameListAttributes(ui->dvdTab_tableView);
        setGameListAttributes(ui->wbfsTab_tableView);

        setFilesColumns();
        setWBFSColumns();

        if (WIIBAFU_SETTINGS.value("Main/GameLanguage", QVariant(0)).toInt() != language) {
            updateTitles();

            if (!ui->infoTab_lineEdit_ID->text().isEmpty()) {
                if (ui->infoTab_lineEdit_UsedBlocks->text().contains("--")) {
                    setGameInfoDateTimes(ui->filesTab_tableView, filesListModel);
                }
                else {
                    setGameInfoDateTimes(ui->wbfsTab_tableView, wbfsListModel);
                }
            }
        }

        ui->infoTab_comboBox_GameLanguages->setCurrentIndex(WIIBAFU_SETTINGS.value("Main/GameLanguage", QVariant(0)).toInt());

        if (WIIBAFU_SETTINGS.value("GameListBehavior/ToolTips", QVariant(false)).toBool()) {
            setToolTips(ui->filesTab_tableView, filesListModel, tr("Title"), tr("Name"));
            setToolTips(ui->wbfsTab_tableView, wbfsListModel, tr("Title"), tr("Name"));
            ui->filesTab_tableView->update();
            ui->wbfsTab_tableView->update();
        }

        #ifdef Q_OS_MACX
            if (macOSXStyle != WIIBAFU_SETTINGS.value("Main/MacOSXStyle", QVariant("Aqua")).toString()) {
                setMacOSXStyle();
                settings->setMacOSXStyle();
                coverViewDialog->setMacOSXStyle();
                wbfsDialog->setMacOSXStyle();
                wiibafudialog->setMacOSXStyle();
            }
        #endif
    }
}

void WiiBaFu::tools_CheckWBFS_triggered() {
    on_wbfsTab_pushButton_Check_clicked();
}

void WiiBaFu::tools_DumpWBFS_triggered() {
    QtConcurrent::run(wiTools, &WiTools::dumpWBFS, wbfsPath());
}

void WiiBaFu::tools_CreateWBFS_triggered() {
    if (wbfsDialog->exec() == QDialog::Accepted) {
        emit startBusy();

        WiTools::CreateWBFSParameters parameters;
        parameters.Path = wbfsDialog->path();
        parameters.Size = wbfsDialog->size();
        parameters.SplitSize = wbfsDialog->splitSize();
        parameters.HDSectorSize = wbfsDialog->hdSectorSize();
        parameters.WBFSSectorSize = wbfsDialog->wbfsSectorSize();
        parameters.Recover = wbfsDialog->recover();
        parameters.Inode = wbfsDialog->inode();
        parameters.Test = wbfsDialog->test();

        QtConcurrent::run(wiTools, &WiTools::createWBFS, parameters);
    }
}

void WiiBaFu::tools_VerifyGame_triggered() {
    if (!action_VerifyGame->text().contains(tr("&Cancel verifying"))) {
        QString game;

        switch (ui->stackedWidget->currentIndex()) {
            case 0:
                    if (filesListModel->rowCount() > 0 && !ui->filesTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
                        game = filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(10).first())->text();
                    }
                    break;
            case 1:
                    if (dvdListModel->rowCount() > 0) {
                        game = dvdListModel->index(15, 0).data().toString();
                    }
                    break;
            case 2:
                    if (wbfsListModel->rowCount() > 0 && !ui->wbfsTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
                        game = wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(0).first())->text();
                    }
                    break;
        }

        if (!game.isEmpty()) {
            emit startBusy();

            action_VerifyGame->setIcon(QIcon(":/images/cancel.png"));
            action_VerifyGame->setText(tr("&Cancel verifying"));

            QtConcurrent::run(wiTools, &WiTools::verifyGame, ui->stackedWidget->currentIndex(), wbfsPath(), game);
            ui->stackedWidget->setCurrentIndex(4);
        }
        else {
            setStatusBarText(tr("Verify error: No game selected!"));
            addEntryToLog(tr("Verify error: No game selected!"), WiTools::Error);
        }
    }
    else {
        emit cancelVerifying();
    }
}

void WiiBaFu::tools_Compare_triggered() {
    if ((filesListModel->rowCount() > 0 && wbfsListModel->rowCount() > 0) && (ui->stackedWidget->currentIndex() == 0 || ui->stackedWidget->currentIndex() == 2)) {
        QTableView *tableView;
        QStandardItemModel *sourceModel, *targetModel;

        switch (ui->stackedWidget->currentIndex()) {
            case 0:
                    tableView = ui->filesTab_tableView;
                    sourceModel = filesListModel;
                    targetModel = wbfsListModel;
                    break;
            case 2:
                    tableView = ui->wbfsTab_tableView;
                    sourceModel = wbfsListModel;
                    targetModel = filesListModel;
                    break;
            default:
                    return;
        }

        tableView->selectionModel()->clearSelection();
        tableView->setSelectionMode(QAbstractItemView::MultiSelection);

        for (int i = 0; sourceModel->rowCount() > i; ++i) {
            if (!gameInList(targetModel, sourceModel->item(i, 0)->text())) {
                tableView->selectRow(i);
            }
        }

        tableView->setFocus();
        tableView->setSelectionMode((QAbstractItemView::SelectionMode)WIIBAFU_SETTINGS.value("GameListBehavior/SelectionMode", QVariant(QAbstractItemView::ExtendedSelection)).toInt());
    }
}

void WiiBaFu::tools_UpdateTitles_triggered() {
    QtConcurrent::run(common, &Common::updateTitles);
}

void WiiBaFu::on_filesTab_tableView_doubleClicked(QModelIndex) {
    setGameInfo(ui->filesTab_tableView, filesListModel);
}

void WiiBaFu::on_wbfsTab_tableView_doubleClicked(QModelIndex) {
    setGameInfo(ui->wbfsTab_tableView, wbfsListModel);
}

void WiiBaFu::on_filesTab_pushButton_Load_clicked() {
    if (!ui->filesTab_pushButton_Load->text().contains(tr("&Cancel loading"))) {
        QString directory = QFileDialog::getExistingDirectory(this, tr("Open directory"), WIIBAFU_SETTINGS.value("Main/LastFilesPath", QVariant(QDir::homePath())).toString(), QFileDialog::ShowDirsOnly);

        if (!directory.isEmpty()) {
            emit startBusy();

            ui->filesTab_pushButton_Load->setIcon(QIcon(":/images/cancel.png"));
            ui->filesTab_pushButton_Load->setText(tr("&Cancel loading"));
            WIIBAFU_SETTINGS.setValue("Main/LastFilesPath", directory);
            int depth = WIIBAFU_SETTINGS.value("WIT/RecurseDepth", QVariant(10)).toInt();

            QtConcurrent::run(wiTools, &WiTools::requestFilesGameListModel, filesListModel, directory, depth);
        }
    }
    else {
        emit cancelLoading();
    }
}

void WiiBaFu::on_filesTab_pushButton_SelectAll_clicked() {
    if (ui->filesTab_tableView->model()) {
        if (ui->filesTab_tableView->selectionModel()->selectedRows(0).count() != ui->filesTab_tableView->model()->rowCount()) {
            ui->filesTab_tableView->selectAll();
        }
        else {
            ui->filesTab_tableView->selectionModel()->clearSelection();
        }
    }
}

void WiiBaFu::on_filesTab_pushButton_TransferToWBFS_clicked() {
    filesTab_TransferToWBFS(false);
}

void WiiBaFu::on_filesTab_pushButton_TransferToImage_clicked() {
    filesTab_TransferToImage(false);
}

void WiiBaFu::on_filesTab_pushButton_ExtractImage_clicked() {
    filesTab_ExtractImage(false);
}

void WiiBaFu::on_filesTab_pushButton_ShowInfo_clicked() {
    setGameInfo(ui->filesTab_tableView, filesListModel);
}

void WiiBaFu::on_dvdTab_pushButton_Load_clicked() {
    emit startBusy();

    QString dvdPath = WIIBAFU_SETTINGS.value("WIT/DVDDrivePath", QVariant("/cdrom")).toString();
    QtConcurrent::run(wiTools, &WiTools::requestDVDGameListModel, dvdListModel, dvdPath);
}

void WiiBaFu::on_dvdTab_pushButton_TransferToWBFS_clicked() {
    if (!ui->dvdTab_pushButton_TransferToWBFS->text().contains(tr("&Cancel transfering"))) {
        if (dvdListModel->rowCount() > 0) {
            WiTools::GamePatchParameters patchParameters;
            patchParameters.Patch = false;

            if (ui->dvdTab_pushButton_Patch->isChecked()) {
                wiibafudialog->setPatchGame();
                wiibafudialog->setGameID(dvdListModel->index(0, 0).data().toString());
                wiibafudialog->setGameName(dvdListModel->index(1, 0).data().toString());

                if (wiibafudialog->exec() == QDialog::Accepted) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == dvdListModel->index(0, 0).data().toString()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == dvdListModel->index(1, 0).data().toString()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }
                else {
                    return;
                }
            }

            ui->dvdTab_pushButton_TransferToWBFS->setIcon(QIcon(":/images/cancel.png"));
            ui->dvdTab_pushButton_TransferToWBFS->setText(tr("&Cancel transfering"));
            QtConcurrent::run(wiTools, &WiTools::transferDVDToWBFS, dvdListModel->index(15, 0).data().toString(), wbfsPath(), patchParameters);
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::on_dvdTab_pushButton_TransferToImage_clicked() {
    if (!ui->dvdTab_pushButton_TransferToImage->text().contains(tr("&Cancel transfering"))) {
        if (dvdListModel->rowCount() > 0) {
            wiibafudialog->setOpenFile(ui->dvdTab_pushButton_Patch->isChecked());
            wiibafudialog->setGameID(dvdListModel->index(0, 0).data().toString());
            wiibafudialog->setGameName(dvdListModel->index(1, 0).data().toString());

            if (wiibafudialog->exec() == QDialog::Accepted && !wiibafudialog->filePath().isEmpty()) {
                WiTools::GamePatchParameters patchParameters;
                WiTools::TransferFilesToImageParameters transferParameters;
                patchParameters.Patch = false;

                if (ui->dvdTab_pushButton_Patch->isChecked()) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == dvdListModel->index(0, 0).data().toString()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == dvdListModel->index(1, 0).data().toString()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }

                transferParameters.Directory = wiibafudialog->filePath();
                transferParameters.Format = wiibafudialog->imageFormat();
                transferParameters.Compression = wiibafudialog->compression();
                transferParameters.SplitSize = "";
                transferParameters.PatchParameters = patchParameters;

                if (wiibafudialog->split()) {
                    transferParameters.SplitSize = wiibafudialog->splitSize();
                }

                ui->dvdTab_pushButton_TransferToImage->setIcon(QIcon(":/images/cancel.png"));
                ui->dvdTab_pushButton_TransferToImage->setText(tr("&Cancel transfering"));
                QtConcurrent::run(wiTools, &WiTools::transferDVDToImage, WIIBAFU_SETTINGS.value("WIT/DVDDrivePath", QVariant("/cdrom")).toString(), transferParameters);
            }
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::on_dvdTab_pushButton_Extract_clicked() {
    if (!ui->dvdTab_pushButton_Extract->text().contains(tr("&Cancel extracting"))) {
        if (dvdListModel->rowCount() > 0) {
            wiibafudialog->setOpenDirectory(ui->dvdTab_pushButton_Patch->isChecked());
            wiibafudialog->setGameID(dvdListModel->index(0, 0).data().toString());
            wiibafudialog->setGameName(dvdListModel->index(1, 0).data().toString());

            if (wiibafudialog->exec() == QDialog::Accepted && !wiibafudialog->directory().isEmpty()) {
                WiTools::GamePatchParameters patchParameters;
                patchParameters.Patch = false;

                if (ui->dvdTab_pushButton_Patch->isChecked()) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == dvdListModel->index(0, 0).data().toString()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == dvdListModel->index(1, 0).data().toString()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }

                ui->dvdTab_pushButton_Extract->setIcon(QIcon(":/images/cancel.png"));
                ui->dvdTab_pushButton_Extract->setText(tr("&Cancel extracting"));
                QtConcurrent::run(wiTools, &WiTools::extractDVD, WIIBAFU_SETTINGS.value("WIT/DVDDrivePath", QVariant("/cdrom")).toString(), buildPath(wiibafudialog->directory(), dvdListModel, ui->dvdTab_tableView, patchParameters.ID), patchParameters);
            }
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::on_wbfsTab_pushButton_Load_clicked() {
    emit startBusy();

    //QtConcurrent::run(wiTools, &WiTools::requestWBFSGameListModel, wbfsListModel, wbfsPath());
    wiTools->requestWBFSGameListModel(wbfsListModel, wbfsPath());
}

void WiiBaFu::on_wbfsTab_pushButton_SelectAll_clicked() {
    if (ui->wbfsTab_tableView->model()) {
        if (ui->wbfsTab_tableView->selectionModel()->selectedRows(0).count() != ui->wbfsTab_tableView->model()->rowCount()) {
            ui->wbfsTab_tableView->selectAll();
        }
        else {
            ui->wbfsTab_tableView->selectionModel()->clearSelection();
        }
    }
}

void WiiBaFu::on_wbfsTab_pushButton_Transfer_clicked() {
    wbfsTab_Transfer(false);
}

void WiiBaFu::on_wbfsTab_pushButton_Extract_clicked() {
    wbfsTab_Extract(false);
}

void WiiBaFu::on_wbfsTab_pushButton_Remove_clicked() {
    if (ui->wbfsTab_tableView->model() && !ui->wbfsTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
        int result = QMessageBox::warning(this, tr("Remove games"), tr("Are you sure that you want to delete the selected games?\n\nAttention:\nThe selected games are finally deleted from the WBFS file system!"), QMessageBox::Ok, QMessageBox::Cancel);
        if (result == QMessageBox::Ok) {
            QtConcurrent::run(wiTools, &WiTools::removeGamesFromWBFS, ui->wbfsTab_tableView->selectionModel()->selectedRows(0), wbfsPath());
        }
    }
}

void WiiBaFu::on_wbfsTab_pushButton_ShowInfo_clicked() {
    setGameInfo(ui->wbfsTab_tableView, wbfsListModel);
}

void WiiBaFu::on_wbfsTab_pushButton_Check_clicked() {
    int result = QMessageBox::question(this, tr("Check/Repair WBFS"), tr("Are you sure that you want to check/repair the wbfs?"), QMessageBox::Ok, QMessageBox::Cancel);
    if (result == QMessageBox::Ok) {
        QtConcurrent::run(wiTools, &WiTools::checkWBFS, wbfsPath());
    }
}

void WiiBaFu::on_infoTab_pushButton_Load3DCover_clicked() {
    if (!ui->infoTab_lineEdit_ID->text().isEmpty()) {
        ui->infoTab_label_GameCover->clear();
        common->requestGameCover(ui->infoTab_lineEdit_ID->text(), currentGameLanguage(), Common::ThreeD);
    }
}

void WiiBaFu::on_infoTab_pushButton_LoadFullHQCover_clicked() {
    if (!ui->infoTab_lineEdit_ID->text().isEmpty()) {
        common->requestGameCover(ui->infoTab_lineEdit_ID->text(), currentGameLanguage(), Common::HighQuality);
    }
}

void WiiBaFu::on_infoTab_pushButton_viewInBrowser_clicked() {
    if (!ui->infoTab_lineEdit_ID->text().isEmpty()) {
        common->viewInBrowser(ui->infoTab_lineEdit_ID->text());
    }
}

void WiiBaFu::on_infoTab_pushButton_Reset_clicked() {
    ui->toolBar->actions().at(3)->setText(tr("Info"));

    ui->infoTab_lineEdit_ID->clear();
    ui->infoTab_lineEdit_Name->clear();
    ui->infoTab_lineEdit_Title->clear();
    ui->infoTab_lineEdit_Region->clear();
    ui->infoTab_lineEdit_Size->clear();
    ui->infoTab_lineEdit_UsedBlocks->clear();
    ui->infoTab_lineEdit_Insertion->clear();
    ui->infoTab_lineEdit_LastModification->clear();
    ui->infoTab_lineEdit_LastStatusChange->clear();
    ui->infoTab_lineEdit_LastAccess->clear();
    ui->infoTab_lineEdit_Type->clear();
    ui->infoTab_lineEdit_WBFSSlot->clear();
    ui->infoTab_lineEdit_Source->clear();

    ui->infoTab_label_GameCover->clear();
}

void WiiBaFu::on_logTab_pushButton_Clear_clicked() {
    ui->logTab_plainTextEdit_Log->clear();
}

void WiiBaFu::on_logTab_pushButton_Copy_clicked() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->logTab_plainTextEdit_Log->toPlainText());
}

void WiiBaFu::on_logTab_pushButton_Find_clicked() {
    bool result;
    QString lastSearchString = WIIBAFU_SETTINGS.value("Main/LastLogSearchString", QVariant("")).toString();
    QString searchString = QInputDialog::getText(this, tr("Search log"), tr("Enter search string:"), QLineEdit::Normal, lastSearchString, &result);

    if (result && !searchString.isEmpty()) {
        WIIBAFU_SETTINGS.setValue("Main/LastLogSearchString", searchString);

        bool found = false;

        if (!logFindFirstTime) {
            ui->logTab_plainTextEdit_Log->document()->undo();
        }

        QTextCursor highlightCursor(ui->logTab_plainTextEdit_Log->document());
        QTextCursor cursor(ui->logTab_plainTextEdit_Log->document());
        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setForeground(Qt::red);

        cursor.beginEditBlock();

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = ui->logTab_plainTextEdit_Log->document()->find(searchString, highlightCursor, QTextDocument::FindWholeWords);

            if (!highlightCursor.isNull()) {
                found = true;
                highlightCursor.movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();
        logFindFirstTime = false;

        if (!found) {
            setStatusBarText(tr("Nothing found!"));
            logFindFirstTime = true;
        }
    }
}

void WiiBaFu::on_logTab_pushButton_Save_clicked() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save log file"), QDir::homePath().append("/WiiBaFu.log"), tr("WiiBaFu log file (*.log)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);

        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream textStream(&file);
            textStream << ui->logTab_plainTextEdit_Log->toPlainText();
        }

        file.close();
    }
}

void WiiBaFu::filesTab_ContextMenu_TransferToWBFS_Patch() {
    filesTab_TransferToWBFS(true);
}

void WiiBaFu::filesTab_ContextMenu_TransferToImage_Patch() {
    filesTab_TransferToImage(true);
}

void WiiBaFu::filesTab_ContextMenu_ExtractImage_Path() {
    filesTab_ExtractImage(true);
}

void WiiBaFu::filesTab_TransferToWBFS(const bool patch) {
    if (!ui->filesTab_pushButton_TransferToWBFS->text().contains(tr("&Cancel transfering"))) {
        if (ui->filesTab_tableView->selectionModel() && !ui->filesTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
            WiTools::GamePatchParameters parameters;
            parameters.Patch = false;

            if (patch) {
                wiibafudialog->setPatchGame();
                wiibafudialog->setGameID(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text());
                wiibafudialog->setGameName(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text());

                if (wiibafudialog->exec() == QDialog::Accepted) {
                    parameters.Patch = true;

                    if (wiibafudialog->gameID() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text()) {
                        parameters.ID = "";
                    }
                    else {
                        parameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text()) {
                        parameters.Name = "";
                    }
                    else {
                        parameters.Name = wiibafudialog->gameName();
                    }

                    parameters.Region = wiibafudialog->gameRegion();
                    parameters.IOS = wiibafudialog->gameIOS();
                    parameters.Modify = wiibafudialog->gameModify();
                    parameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    parameters.CommonKey = wiibafudialog->gameCommonKey();
                }
                else {
                    return;
                }
            }

            ui->filesTab_pushButton_TransferToWBFS->setIcon(QIcon(":/images/cancel.png"));
            ui->filesTab_pushButton_TransferToWBFS->setText(tr("&Cancel transfering"));
            QtConcurrent::run(wiTools, &WiTools::transferFilesToWBFS, ui->filesTab_tableView->selectionModel()->selectedRows(10), wbfsPath(), parameters);
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::filesTab_TransferToImage(const bool patch) {
    if (!ui->filesTab_pushButton_TransferToImage->text().contains(tr("&Cancel transfering"))) {
        if (ui->filesTab_tableView->model() && !ui->filesTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
            const int row = ui->filesTab_tableView->selectionModel()->selectedRows().first().row();
            const QString imagePath = filesListModel->item(row, 10)->data(Qt::DisplayRole).toString();

            wiibafudialog->setSourceFilePath(imagePath);
            wiibafudialog->setDirectory(lastOutputPath);
            wiibafudialog->setOpenImageDirectory(patch);
            wiibafudialog->setGameID(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text());
            wiibafudialog->setGameName(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text());

            if (wiibafudialog->exec() == QDialog::Accepted && !wiibafudialog->directory().isEmpty()) {
                WiTools::GamePatchParameters patchParameters;
                WiTools::TransferFilesToImageParameters transferParameters;
                patchParameters.Patch = false;

                if (patch) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }

                QDir path = wiibafudialog->directory();
                QStringList filenames;

                foreach (QModelIndex index, ui->filesTab_tableView->selectionModel()->selectedRows(10)) {
                    filenames.append(index.data().toString());
                }

                transferParameters.fileList = filenames;
                transferParameters.Directory = path.absolutePath();
                transferParameters.Format = wiibafudialog->imageFormat();
                transferParameters.Compression = wiibafudialog->compression();
                transferParameters.SplitSize = "";
                transferParameters.PatchParameters = patchParameters;

                if (wiibafudialog->split()) {
                    transferParameters.SplitSize = wiibafudialog->splitSize();
                }

                lastOutputPath = wiibafudialog->directory();

                WIIBAFU_SETTINGS.setValue("Main/LastOutputPath", lastOutputPath);

                if (!path.exists()) {
                    QMessageBox::warning(this, tr("Warning"), tr("The directory doesn't exists!"), QMessageBox::Ok, QMessageBox::NoButton);
                }
                else {
                    ui->filesTab_pushButton_TransferToImage->setIcon(QIcon(":/images/cancel.png"));
                    ui->filesTab_pushButton_TransferToImage->setText(tr("&Cancel transfering"));

                    QtConcurrent::run(wiTools, &WiTools::transferFilesToImage, transferParameters);
                }
            }
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::filesTab_ExtractImage(const bool patch) {
    if (!ui->filesTab_pushButton_ExtractImage->text().contains(tr("&Cancel extracting"))) {
        if (filesListModel->rowCount() > 0 && !ui->filesTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
            wiibafudialog->setOpenDirectory(patch);
            wiibafudialog->setGameID(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text());
            wiibafudialog->setGameName(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text());

            if (wiibafudialog->exec() == QDialog::Accepted && !wiibafudialog->directory().isEmpty()) {
                WiTools::GamePatchParameters patchParameters;
                patchParameters.Patch = false;

                if (patch) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }

                QDir path = wiibafudialog->directory();

                if (!path.exists()) {
                    QMessageBox::warning(this, tr("Warning"), tr("The directory doesn't exists!"), QMessageBox::Ok, QMessageBox::NoButton);
                }
                else {
                    ui->filesTab_pushButton_ExtractImage->setIcon(QIcon(":/images/cancel.png"));
                    ui->filesTab_pushButton_ExtractImage->setText(tr("&Cancel extracting"));
                    QtConcurrent::run(wiTools, &WiTools::extractImage, ui->filesTab_tableView->selectionModel()->selectedRows(10), buildPath(path.absolutePath(), filesListModel, ui->filesTab_tableView, patchParameters.ID), patchParameters);
                }
            }
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::wbfsTab_ContextMenu_Transfer_Patch() {
    wbfsTab_Transfer(true);
}

void WiiBaFu::wbfsTab_ContextMenu_Extract_Path() {
    wbfsTab_Extract(true);
}

void WiiBaFu::wbfsTab_Transfer(const bool patch) {
    if (!ui->wbfsTab_pushButton_Transfer->text().contains(tr("&Cancel transfering"))) {
        if (ui->wbfsTab_tableView->model() && !ui->wbfsTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
            wiibafudialog->setOpenImageDirectory(patch);
            wiibafudialog->setGameID(wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(0).first())->text());
            wiibafudialog->setGameName(wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(1).first())->text());

            if (wiibafudialog->exec() == QDialog::Accepted && !wiibafudialog->directory().isEmpty()) {
                WiTools::GamePatchParameters patchParameters;
                WiTools::TransferFilesToImageParameters transferParameters;
                patchParameters.Patch = false;

                if (patch) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(0).first())->text()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(1).first())->text()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }

                QDir path = wiibafudialog->directory();
                QStringList filenames;

                foreach (QModelIndex index, ui->filesTab_tableView->selectionModel()->selectedRows(10)) {
                    filenames.append(index.data().toString());
                }

                transferParameters.fileList = filenames;
                transferParameters.Directory = path.absolutePath();
                transferParameters.Format = wiibafudialog->imageFormat();
                transferParameters.Compression = wiibafudialog->compression();
                transferParameters.SplitSize = "";
                transferParameters.PatchParameters = patchParameters;

                if (wiibafudialog->split()) {
                    transferParameters.SplitSize = wiibafudialog->splitSize();
                }

                if (!path.exists() && !wiibafudialog->directory().isEmpty()) {
                    QMessageBox::warning(this, tr("Warning"), tr("The directory doesn't exists!"), QMessageBox::Ok, QMessageBox::NoButton);
                }
                else {
                    ui->wbfsTab_pushButton_Transfer->setIcon(QIcon(":/images/cancel.png"));
                    ui->wbfsTab_pushButton_Transfer->setText(tr("&Cancel transfering"));
                    QtConcurrent::run(wiTools, &WiTools::transferWBFSToImage, wbfsPath(), transferParameters);
                }
            }
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::wbfsTab_Extract(const bool patch) {
    if (!ui->wbfsTab_pushButton_Extract->text().contains(tr("&Cancel extracting"))) {
        if (wbfsListModel->rowCount() > 0 && !ui->wbfsTab_tableView->selectionModel()->selectedRows(0).isEmpty()) {
            wiibafudialog->setOpenDirectory(patch);
            wiibafudialog->setGameID(wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(0).first())->text());
            wiibafudialog->setGameName(wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(1).first())->text());

            if (wiibafudialog->exec() == QDialog::Accepted && !wiibafudialog->directory().isEmpty()) {
                WiTools::GamePatchParameters patchParameters;
                patchParameters.Patch = false;

                if (patch) {
                    patchParameters.Patch = true;

                    if (wiibafudialog->gameID() == wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(0).first())->text()) {
                        patchParameters.ID = "";
                    }
                    else {
                        patchParameters.ID = wiibafudialog->gameID();
                    }

                    if (wiibafudialog->gameName() == wbfsListModel->itemFromIndex(ui->wbfsTab_tableView->selectionModel()->selectedRows(1).first())->text()) {
                        patchParameters.Name = "";
                    }
                    else {
                        patchParameters.Name = wiibafudialog->gameName();
                    }

                    patchParameters.Region = wiibafudialog->gameRegion();
                    patchParameters.IOS = wiibafudialog->gameIOS();
                    patchParameters.Modify = wiibafudialog->gameModify();
                    patchParameters.EncodingMode = wiibafudialog->gameEncodingMode();
                    patchParameters.CommonKey = wiibafudialog->gameCommonKey();
                }

                QDir path = wiibafudialog->directory();

                if (!path.exists()) {
                    QMessageBox::warning(this, tr("Warning"), tr("The directory doesn't exists!"), QMessageBox::Ok, QMessageBox::NoButton);
                }
                else {
                    ui->wbfsTab_pushButton_Extract->setIcon(QIcon(":/images/cancel.png"));
                    ui->wbfsTab_pushButton_Extract->setText(tr("&Cancel extracting"));
                    QtConcurrent::run(wiTools, &WiTools::extractWBFS, ui->wbfsTab_tableView->selectionModel()->selectedRows(0), wbfsPath(), wiibafudialog->directory(), patchParameters);
                }
            }
        }
    }
    else {
        emit cancelTransfer();
    }
}

void WiiBaFu::filesTableView_selectionChanged(const QItemSelection, const QItemSelection) {
    filesListModel->setHeaderData(0, Qt::Horizontal, tr("ID (%1)").arg(ui->filesTab_tableView->selectionModel()->selectedRows().count()));

    if (ui->filesTab_tableView->selectionModel()->selectedRows().count() > 1) {
        action_filesGame_TransferToWBFS_Patch->setEnabled(false);
        action_filesGame_TransferToImage_Patch->setEnabled(false);
        action_filesGame_ExtractImage_Patch->setEnabled(false);
        action_filesGame_Patch->setEnabled(false);
    }
    else {
        action_filesGame_TransferToWBFS_Patch->setEnabled(true);
        action_filesGame_TransferToImage_Patch->setEnabled(true);
        action_filesGame_ExtractImage_Patch->setEnabled(true);
        action_filesGame_Patch->setEnabled(true);
    }
}

void WiiBaFu::wbfsTableView_selectionChanged(const QItemSelection, const QItemSelection) {
    wbfsListModel->setHeaderData(0, Qt::Horizontal, tr("ID (%1)").arg(ui->wbfsTab_tableView->selectionModel()->selectedRows().count()));

    if (ui->wbfsTab_tableView->selectionModel()->selectedRows().count() > 1) {
        action_wbfsGame_Transfer_Patch->setEnabled(false);
        action_wbfsGame_Extract_Patch->setEnabled(false);
    }
    else {
        action_wbfsGame_Transfer_Patch->setEnabled(true);
        action_wbfsGame_Extract_Patch->setEnabled(true);
    }
}

void WiiBaFu::setFilesGameListModel() {
    if (filesListModel->rowCount() > 0 ) {
        ui->filesTab_tableView->setModel(filesListModel);
        ui->toolBar->actions().at(0)->setText(tr("Files (%1)").arg(ui->filesTab_tableView->model()->rowCount()));

        ui->filesTab_tableView->horizontalHeader()->restoreState(WIIBAFU_SETTINGS.value("FilesGameList/HeaderStates").toByteArray());

        setFilesColumns();
        if (WIIBAFU_SETTINGS.value("GameListBehavior/ToolTips", QVariant(false)).toBool()) {
            setToolTips(ui->filesTab_tableView, filesListModel, tr("Title"), tr("Name"));
        }

        connect(ui->filesTab_tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(filesTableView_selectionChanged(QItemSelection, QItemSelection)));

        emit stopBusy();
        ui->filesTab_pushButton_Load->setIcon(QIcon(":/images/open.png"));
        ui->filesTab_pushButton_Load->setText(tr("&Load"));
        ui->filesTab_pushButton_Load->setShortcut(tr("Ctrl+L"));
        setStatusBarText(tr("Ready."));
    }
}

void WiiBaFu::loadingGamesCanceled() {
    ui->filesTab_pushButton_Load->setIcon(QIcon(":/images/open.png"));
    ui->filesTab_pushButton_Load->setText(tr("&Load"));
    ui->filesTab_pushButton_Load->setShortcut(tr("Ctrl+L"));

    setStatusBarText(tr("Loading canceled!"));
    emit stopBusy();
}

void WiiBaFu::loadingGamesFailed(WiTools::WitStatus) {
    emit stopBusy();
    ui->filesTab_pushButton_Load->setText(tr("&Load"));
    ui->filesTab_pushButton_Load->setShortcut(tr("Ctrl+L"));
}

void WiiBaFu::setDVDGameListModel() {
    if (dvdListModel->rowCount() > 0) {
        QString dvdPath = WIIBAFU_SETTINGS.value("WIT/DVDDrivePath", QVariant("/cdrom")).toString();

        ui->dvdTab_tableView->setModel(dvdListModel);
        ui->toolBar->actions().at(1)->setText(QString("DVD (%1)").arg(dvdPath));

        emit stopBusy();
        common->requestGameCover(dvdListModel->item(0, 0)->text(), QString("EN"), Common::Disc);
    }
}

void WiiBaFu::setWBFSGameListModel() {
    if (wbfsListModel->rowCount() > 0) {
        ui->wbfsTab_tableView->setModel(wbfsListModel);
        ui->toolBar->actions().at(2)->setText(QString("WBFS (%1)").arg(ui->wbfsTab_tableView->model()->rowCount()));

        ui->wbfsTab_tableView->horizontalHeader()->restoreState(WIIBAFU_SETTINGS.value("WBFSGameList/HeaderStates").toByteArray());

        setWBFSColumns();
        if (WIIBAFU_SETTINGS.value("GameListBehavior/ToolTips", QVariant(false)).toBool()) {
            setToolTips(ui->wbfsTab_tableView, wbfsListModel, tr("Title"), tr("Name"));
        }

        connect(ui->wbfsTab_tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(wbfsTableView_selectionChanged(QItemSelection, QItemSelection)));

        emit stopBusy();
        setStatusBarText(tr("Ready."));
    }
}

void WiiBaFu::transferFilesToWBFS_finished(WiTools::WitStatus status) {
    if (status == WiTools::Ok) {
        on_wbfsTab_pushButton_Load_clicked();
    }

    ui->filesTab_pushButton_TransferToWBFS->setIcon(QIcon(":/images/transfer-to-wbfs.png"));
    ui->filesTab_pushButton_TransferToWBFS->setText(tr("Transfer to &WBFS"));
}

void WiiBaFu::transferFilesToImage_finished(WiTools::WitStatus) {
    ui->filesTab_pushButton_TransferToImage->setIcon(QIcon(":/images/transfer-to-image.png"));
    ui->filesTab_pushButton_TransferToImage->setText(tr("Transfer to &image"));
}

void WiiBaFu::extractImage_finished(WiTools::WitStatus) {
    ui->filesTab_pushButton_ExtractImage->setIcon(QIcon(":/images/extract-image.png"));
    ui->filesTab_pushButton_ExtractImage->setText(tr("E&xtract image"));
}

void WiiBaFu::transferDVDToWBFS_finished(WiTools::WitStatus status) {
    if (status == WiTools::Ok) {
        on_wbfsTab_pushButton_Load_clicked();
    }

    ui->dvdTab_pushButton_TransferToWBFS->setIcon(QIcon(":/images/transfer-to-wbfs.png"));
    ui->dvdTab_pushButton_TransferToWBFS->setText(tr("Transfer to &WBFS"));
}

void WiiBaFu::transferDVDToImage_finished(WiTools::WitStatus) {
    ui->dvdTab_pushButton_TransferToImage->setIcon(QIcon(":/images/transfer-to-image.png"));
    ui->dvdTab_pushButton_TransferToImage->setText(tr("Transfer to &image"));
}

void WiiBaFu::extractDVD_finished(WiTools::WitStatus) {
    ui->dvdTab_pushButton_Extract->setIcon(QIcon(":/images/dvd-extract.png"));
    ui->dvdTab_pushButton_Extract->setText(tr("E&xtract"));
}

void WiiBaFu::transferWBFSToImage_finished(WiTools::WitStatus) {
    ui->wbfsTab_pushButton_Transfer->setIcon(QIcon(":/images/transfer-to-image.png"));
    ui->wbfsTab_pushButton_Transfer->setText(tr("&Transfer"));
}

void WiiBaFu::extractWBFS_finished(WiTools::WitStatus) {
    ui->wbfsTab_pushButton_Extract->setIcon(QIcon(":/images/hdd-extract.png"));
    ui->wbfsTab_pushButton_Extract->setText(tr("E&xtract"));
}

void WiiBaFu::verifyGame_finished(WiTools::WitStatus) {
    emit stopBusy();

    action_VerifyGame->setIcon(QIcon(":/images/verify-game.png"));
    action_VerifyGame->setText(tr("&Verify game"));
    ui->stackedWidget->setCurrentIndex(4);
}

void WiiBaFu::filesGame_Patch() {
    wiibafudialog->setPatchGame();
    wiibafudialog->setGameID(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text());
    wiibafudialog->setGameName(filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text());

    if ( wiibafudialog->exec() == QDialog::Accepted) {
        WiTools::GamePatchParameters parameters;

        if (wiibafudialog->gameID() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(0).first())->text()) {
            parameters.ID = "";
        }
        else {
            parameters.ID = wiibafudialog->gameID();
        }

        if (wiibafudialog->gameName() == filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(1).first())->text()) {
            parameters.Name = "";
        }
        else {
            parameters.Name = wiibafudialog->gameName();
        }

        parameters.Region = wiibafudialog->gameRegion();
        parameters.IOS = wiibafudialog->gameIOS();
        parameters.Modify = wiibafudialog->gameModify();
        parameters.EncodingMode = wiibafudialog->gameEncodingMode();
        parameters.CommonKey = wiibafudialog->gameCommonKey();

        QtConcurrent::run(wiTools, &WiTools::patchGameImage, filesListModel->itemFromIndex(ui->filesTab_tableView->selectionModel()->selectedRows(10).first())->text(), parameters);
    }
}

void WiiBaFu::filesGame_Patch_finished(WiTools::WitStatus status) {
    if (status == WiTools::Ok) {
        emit startBusy();
        ui->filesTab_pushButton_Load->setText(tr("&Cancel loading"));
        QtConcurrent::run(wiTools, &WiTools::requestFilesGameListModel, filesListModel, WIIBAFU_SETTINGS.value("Main/LastFilesPath", QVariant(QDir::homePath())).toString(), WIIBAFU_SETTINGS.value("WIT/RecurseDepth", QVariant(10)).toInt());
    }
}

void WiiBaFu::updateTitles() {
    if (filesListModel->rowCount() > 0) {
        for (int i = 0; i < filesListModel->rowCount(); i++) {
            filesListModel->item(i, 2)->setText(common->titleFromDB(filesListModel->item(i, 0)->text()));
        }

        ui->filesTab_tableView->update();
    }

    if (wbfsListModel->rowCount() > 0) {
        for (int i = 0; i < wbfsListModel->rowCount(); i++) {
            wbfsListModel->item(i, 2)->setText(common->titleFromDB(wbfsListModel->item(i, 0)->text()));
        }

        ui->wbfsTab_tableView->update();
    }
}

QString WiiBaFu::buildPath(const QString directory, QStandardItemModel *model, QTableView *tableView, const QString id) {
    QString path;
    QString gameId;
    QString gameTitle;
    QDir dir(directory);

    if (dir.exists()) {
        if (tableView != ui->dvdTab_tableView) {
            gameId = model->itemFromIndex(tableView->selectionModel()->selectedRows(0).first())->text();
            gameTitle = model->itemFromIndex(tableView->selectionModel()->selectedRows(2).first())->text();
        }
        else {
            gameId = model->index(0, 0).data(Qt::DisplayRole).toString();
            gameTitle = model->index(2, 0).data(Qt::DisplayRole).toString();
        }

        if (!id.isEmpty()) {
            gameId = id;
        }

        path = dir.path().append(QDir::separator()).append(gameTitle).append(" [").append(gameId).append("]");
    }
    else {
        path = dir.path();
    }

    return path;
}

void WiiBaFu::setToolTips(QTableView *tableView, QStandardItemModel *model, const QString firstColumnName, const QString secondColumnName) {
    if (tableView->isColumnHidden(headerIndex(model, firstColumnName, Qt::Horizontal))) {
        for (int i = 0; i < model->rowCount(); i++) {
            QString toolTip = model->item(i, headerIndex(model, firstColumnName, Qt::Horizontal))->text();
            model->item(i, headerIndex(model, firstColumnName, Qt::Horizontal))->setToolTip("");
            model->item(i, headerIndex(model, secondColumnName, Qt::Horizontal))->setToolTip(toolTip);
        }
    }
    else if (tableView->isColumnHidden(headerIndex(model, secondColumnName, Qt::Horizontal))) {
        for (int i = 0; i < model->rowCount(); i++) {
            QString toolTip = model->item(i, headerIndex(model, secondColumnName, Qt::Horizontal))->text();
            model->item(i, headerIndex(model, secondColumnName, Qt::Horizontal))->setToolTip("");
            model->item(i, headerIndex(model, firstColumnName, Qt::Horizontal))->setToolTip(toolTip);
        }
    }
    else {
        for (int i = 0; i < model->rowCount(); i++) {
            model->item(i, headerIndex(model, firstColumnName, Qt::Horizontal))->setToolTip("");
            model->item(i, headerIndex(model, secondColumnName, Qt::Horizontal))->setToolTip("");
        }
    }
}

void WiiBaFu::setGameInfo(QTableView *tableView, QStandardItemModel *model) {
    if (tableView->selectionModel() && !tableView->selectionModel()->selectedRows(0).isEmpty()) {
        ui->stackedWidget->setCurrentIndex(3);

        bool sameGame;
        if (!ui->infoTab_lineEdit_ID->text().contains(model->itemFromIndex(tableView->selectionModel()->selectedRows(0).first())->text())) {
            sameGame = false;
        }
        else {
            sameGame = true;
        }

        ui->toolBar->actions().at(3)->setText(tr("Info (%1)").arg(model->itemFromIndex(tableView->selectionModel()->selectedRows(0).first())->text()));

        if (tableView == ui->wbfsTab_tableView) {
            ui->infoTab_lineEdit_ID->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(0).first())->text());
            ui->infoTab_lineEdit_Name->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(1).first())->text());
            ui->infoTab_lineEdit_Title->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(2).first())->text());
            ui->infoTab_lineEdit_Region->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(3).first())->text());
            ui->infoTab_lineEdit_Size->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(4).first())->text());
            ui->infoTab_lineEdit_UsedBlocks->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(5).first())->text());
            setGameInfoDateTimes(ui->wbfsTab_tableView, wbfsListModel);
            ui->infoTab_lineEdit_Type->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(10).first())->text());
            ui->infoTab_lineEdit_WBFSSlot->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(11).first())->text());
            ui->infoTab_lineEdit_Source->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(12).first())->text());
        }
        else {
            ui->infoTab_lineEdit_ID->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(0).first())->text());
            ui->infoTab_lineEdit_Name->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(1).first())->text());
            ui->infoTab_lineEdit_Title->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(2).first())->text());
            ui->infoTab_lineEdit_Region->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(3).first())->text());
            ui->infoTab_lineEdit_Size->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(4).first())->text());
            ui->infoTab_lineEdit_UsedBlocks->setText("--");
            setGameInfoDateTimes(ui->filesTab_tableView, filesListModel);
            ui->infoTab_lineEdit_Type->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(9).first())->text());
            ui->infoTab_lineEdit_WBFSSlot->setText("--");
            ui->infoTab_lineEdit_Source->setText(model->itemFromIndex(tableView->selectionModel()->selectedRows(10).first())->text());
        }

        if (!sameGame) {
            if (model->itemFromIndex(tableView->selectionModel()->selectedRows(3).first())->text().contains("PAL") || model->itemFromIndex(tableView->selectionModel()->selectedRows(3).first())->text().contains("RF")) {
                ui->infoTab_comboBox_GameLanguages->setCurrentIndex(WIIBAFU_SETTINGS.value("Main/GameLanguage", QVariant(0)).toInt());
            }
            else if (model->itemFromIndex(tableView->selectionModel()->selectedRows(3).first())->text().contains("NTSC")) {
                ui->infoTab_comboBox_GameLanguages->setCurrentIndex(1);
            }

            ui->infoTab_label_GameCover->clear();
            common->requestGameCover(model->itemFromIndex(tableView->selectionModel()->selectedRows(0).first())->text(), currentGameLanguage(), Common::ThreeD);
        }
    }
}

void WiiBaFu::setGameInfoDateTimes(QTableView *tableView, QStandardItemModel *model) {
    QDateTime dateTime;
    QString dateTimeFormat = locale().dateTimeFormat(QLocale::ShortFormat);

    if (tableView == ui->wbfsTab_tableView) {
        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(6).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_Insertion->setText(dateTime.toString(dateTimeFormat));

        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(7).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_LastModification->setText(dateTime.toString(dateTimeFormat));

        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(8).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_LastStatusChange->setText(dateTime.toString(dateTimeFormat));

        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(9).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_LastAccess->setText(dateTime.toString(dateTimeFormat));
    }
    else {
        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(5).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_Insertion->setText(dateTime.toString(dateTimeFormat));

        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(6).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_LastModification->setText(dateTime.toString(dateTimeFormat));

        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(7).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_LastStatusChange->setText(dateTime.toString(dateTimeFormat));

        dateTime = model->itemFromIndex(tableView->selectionModel()->selectedRows(8).first())->data(Qt::DisplayRole).value<QDateTime>();
        ui->infoTab_lineEdit_LastAccess->setText(dateTime.toString(dateTimeFormat));
    }
}

void WiiBaFu::showGameDiscCover(const QImage gameCover) {
    ui->dvdTab_label_DiscCover->setPixmap(QPixmap::fromImage(gameCover, Qt::AutoColor));
}

void WiiBaFu::showGame3DCover(const QImage gameCover) {
    ui->infoTab_label_GameCover->setPixmap(QPixmap::fromImage(gameCover, Qt::AutoColor));
}

void WiiBaFu::showGameFullHQCover(const QImage gameFullHQCover) {
    coverViewDialog->setCover(gameFullHQCover, ui->infoTab_lineEdit_ID->text());
    coverViewDialog->show();
}

void WiiBaFu::setWBFSInfoText(const QString text) {
    ui->wbfsTab_label_Info->setText(text);
}

void WiiBaFu::setWBFSProgressBar(const int min, const int max, const int value, const QString format) {
    ui->wbfsTab_progressBar->setMinimum(min);
    ui->wbfsTab_progressBar->setMaximum(max);
    ui->wbfsTab_progressBar->setValue(value);
    ui->wbfsTab_progressBar->setFormat(format);
}

void WiiBaFu::setMainProgressBarVisible(const bool visible) {
    progressBar_Main->setVisible(visible);

    #ifdef Q_OS_MACX
        if (!visible) {
            setWindowTitle("Wii Backup Fusion " + QCoreApplication::applicationVersion());
        }
    #endif
}

void WiiBaFu::setMainProgressBar(const int value, const QString format) {
    progressBar_Main->setValue(value);
    progressBar_Main->setFormat(format);

    #ifdef Q_OS_MACX
        setWindowTitle(QString(format).replace("%p%", QString::number(value).append("%")));
    #endif
}

void WiiBaFu::startMainProgressBarBusy() {
    timer->start(3000);
}

void WiiBaFu::showMainProgressBarBusy() {
    setMainProgressBarVisible(true);

    progressBar_Main->setMinimum(0);
    progressBar_Main->setMaximum(0);
    progressBar_Main->setValue(-1);
}

void WiiBaFu::stopMainProgressBarBusy() {
    if (timer->isActive()) {
        timer->stop();
    }

    setMainProgressBarVisible(false);

    progressBar_Main->setMinimum(0);
    progressBar_Main->setMaximum(100);
    progressBar_Main->setValue(0);
}

void WiiBaFu::setStatusBarText(const QString text) {
    ui->statusBar->showMessage(text);
}

void WiiBaFu::addEntryToLog(const QString entry, const WiTools::LogType type) {
    switch (WIIBAFU_SETTINGS.value("Main/Logging", QVariant(0)).toInt()) {
        case 0:
                ui->logTab_plainTextEdit_Log->appendPlainText(entry);

                if (logFile.isOpen()) {
                    logFile.write(entry.toLatin1().append("\n"));
                }

                break;
        case 1:
                if (type == WiTools::Error) {
                    ui->logTab_plainTextEdit_Log->appendPlainText(entry);

                    if (!logFile.isOpen()) {
                        logFile.write(entry.toLatin1().append("\n"));
                    }
                }

                break;
    }
}

void WiiBaFu::addEntriesToLog(const QStringList entries, const WiTools::LogType type) {
    QString tmpstr;

    foreach (QString entry, entries) {
        tmpstr.append(entry);
        tmpstr.append(" ");
    }

    addEntryToLog(tmpstr, type);
}

void WiiBaFu::addWitCommandLineToLog(const QString wit, const QStringList arguments) {
    if (WIIBAFU_SETTINGS.value("Main/LogWitCommandLine", QVariant(true)).toBool()) {
        addEntriesToLog(QStringList() << tr("WIT command line:\n%1").arg(wit) << arguments << "\n" , WiTools::Info);
    }
}

QString WiiBaFu::currentGameLanguage() {
    switch (ui->infoTab_comboBox_GameLanguages->currentIndex()) {
        case 0:  return "EN";
                 break;
        case 1:  return "US";
                 break;
        case 2:  return "FR";
                 break;
        case 3:  return "DE";
                 break;
        case 4:  return "ES";
                 break;
        case 5:  return "IT";
                 break;
        case 6:  return "NL";
                 break;
        case 7:  return "PT";
                 break;
        case 8:  return "SE";
                 break;
        case 9:  return "DK";
                 break;
        case 10: return "NO";
                 break;
        case 11: return "FI";
                 break;
        case 12: return "RU";
                 break;
        case 13: return "JA";
                 break;
        case 14: return "KO";
                 break;
        case 15: return "ZH-tw";
                 break;
        case 16: return "ZH-cn";
                 break;
        default: return "EN";
    }
}

QLocale WiiBaFu::locale() {
    QLocale locale;

    switch (WIIBAFU_SETTINGS.value("Main/GameLanguage", QVariant(0)).toInt()) {
        case 0:  locale = QLocale(QLocale::English, QLocale::UnitedKingdom);
                 break;
        case 1:  locale = QLocale(QLocale::English, QLocale::UnitedStates);
                 break;
        case 2:  locale = QLocale(QLocale::French, QLocale::France);
                 break;
        case 3:  locale = QLocale(QLocale::German, QLocale::Germany);
                 break;
        case 4:  locale = QLocale(QLocale::Spanish, QLocale::Spain);
                 break;
        case 5:  locale = QLocale(QLocale::Italian, QLocale::Italy);
                 break;
        case 6:  locale = QLocale(QLocale::Dutch, QLocale::Netherlands);
                 break;
        case 7:  locale = QLocale(QLocale::Portuguese, QLocale::Portugal);
                 break;
        case 8:  locale = QLocale(QLocale::Swedish, QLocale::Sweden);
                 break;
        case 9:  locale = QLocale(QLocale::Danish, QLocale::Denmark);
                 break;
        case 10: locale = QLocale(QLocale::NorthernSami, QLocale::Norway);
                 break;
        case 11: locale = QLocale(QLocale::Finnish, QLocale::Finland);
                 break;
        case 12: locale = QLocale(QLocale::Russian, QLocale::RussianFederation);
                 break;
        case 13: locale = QLocale(QLocale::Japanese, QLocale::Japan);
                 break;
        case 14: locale = QLocale(QLocale::Korean, QLocale::DemocraticRepublicOfKorea);
                 break;
        case 15: locale = QLocale(QLocale::Chinese, QLocale::Taiwan);
                 break;
        case 16: locale = QLocale(QLocale::Chinese, QLocale::China);
                 break;
        default: locale = QLocale(QLocale::English, QLocale::UnitedKingdom);
    }

    return locale;
}

QString WiiBaFu::wbfsPath() {
    if (WIIBAFU_SETTINGS.value("WIT/Auto", QVariant(true)).toBool()) {
        return QString("");
    }
    else {
        return WIIBAFU_SETTINGS.value("WIT/WBFSPath", QVariant("")).toString();
    }
}

int WiiBaFu::headerIndex(QAbstractItemModel *model, const QString text, const Qt::Orientation orientation) {
    for (int i = 0; i < model->columnCount(); i++) {
        if (model->headerData(i, orientation).toString().contains(text)) {
            return i;
        }
    }

    return -1;
}

bool WiiBaFu::gameInList(QStandardItemModel *model, const QString gameId) {
    for (int i = 0; model->rowCount() > i; ++i) {
        if (model->item(i, 0)->text().contains(gameId)) {
            return true;
        }
    }

    return false;
}

void WiiBaFu::saveMainWindowGeometry() {
    WIIBAFU_SETTINGS.setValue("MainWindow/x", this->geometry().x());
    WIIBAFU_SETTINGS.setValue("MainWindow/y", this->geometry().y());
    WIIBAFU_SETTINGS.setValue("MainWindow/width", this->geometry().width());
    WIIBAFU_SETTINGS.setValue("MainWindow/height", this->geometry().height());
}

void WiiBaFu::saveGameListHeaderStates() {
    if (ui->filesTab_tableView->horizontalHeader()->count() > 0 ) {
        WIIBAFU_SETTINGS.setValue("FilesGameList/HeaderStates", ui->filesTab_tableView->horizontalHeader()->saveState());
    }

    if (ui->wbfsTab_tableView->horizontalHeader()->count() > 0 ) {
        WIIBAFU_SETTINGS.setValue("WBFSGameList/HeaderStates", ui->wbfsTab_tableView->horizontalHeader()->saveState());
    }
}

void WiiBaFu::setFilesColumns() {
    if (WIIBAFU_SETTINGS.value("FilesGameList/columnID", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(0);
    }
    else {
        ui->filesTab_tableView->hideColumn(0);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnName", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(1);
    }
    else {
        ui->filesTab_tableView->hideColumn(1);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnTitle", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(2);
    }
    else {
        ui->filesTab_tableView->hideColumn(2);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnRegion", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(3);
    }
    else {
        ui->filesTab_tableView->hideColumn(3);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnSize", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(4);
    }
    else {
        ui->filesTab_tableView->hideColumn(4);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnInsertion", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(5);
    }
    else {
        ui->filesTab_tableView->hideColumn(5);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnLastModification", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(6);
    }
    else {
        ui->filesTab_tableView->hideColumn(6);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnLastStatusChange", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(7);
    }
    else {
        ui->filesTab_tableView->hideColumn(7);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnLastAccess", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(8);
    }
    else {
        ui->filesTab_tableView->hideColumn(8);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnType", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(9);
    }
    else {
        ui->filesTab_tableView->hideColumn(9);
    }

    if (WIIBAFU_SETTINGS.value("FilesGameList/columnSource", QVariant(true)).toBool()) {
        ui->filesTab_tableView->showColumn(10);
    }
    else {
        ui->filesTab_tableView->hideColumn(10);
    }
}

void WiiBaFu::setWBFSColumns() {
    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnID", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(0);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(0);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnName", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(1);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(1);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnTitle", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(2);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(2);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnRegion", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(3);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(3);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnSize", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(4);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(4);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnUsedBlocks", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(5);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(5);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnInsertion", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(6);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(6);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnLastModification", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(7);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(7);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnLastStatusChange", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(8);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(8);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnLastAccess", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(9);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(9);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnType", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(10);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(10);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnWBFSSlot", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(11);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(11);
    }

    if (WIIBAFU_SETTINGS.value("WBFSGameList/columnSource", QVariant(true)).toBool()) {
        ui->wbfsTab_tableView->showColumn(12);
    }
    else {
        ui->wbfsTab_tableView->hideColumn(12);
    }
}

void WiiBaFu::help_About_triggered() {
    QString message = QString("<h2>" + QCoreApplication::applicationName() + " %1</h2>").arg(QCoreApplication::applicationVersion()) +
        "<p><b><i>The complete and simply to use backup solution for Wii games</b></i>"
        "<p>Development and Copyright &copy; 2010 - 2013 Kai Christian Heitkamp"
        "<p><a href='mailto:dynup<dynup@ymail.com>?subject=WiiBaFu%20feedback'>dynup@ymail.com</a>"
        " | <a href='http://dynup.de.vu'>dynup.de.vu</a>"
        "<p><font color='red'>I don't support piracy! If you copy games with this software,"
        "<br>you must have the original and it's for your private use only!</font color>"
        "<p><font color=black>Ctrl / Strg + 1:</font> shows Files"
        "<br><font color=black>Ctrl / Strg + 2:</font> shows DVD"
        "<br><font color=black>Ctrl / Strg + 3:</font> shows WBFS"
        "<br><font color=black>Ctrl / Strg + 4:</font> shows Info"
        "<br><font color=black>Ctrl / Strg + 5:</font> shows Log"
        "<p>Big thanks to the trolls at Trolltech Norway for his excellent Qt toolkit, the guys at Digia for the continuation and all people at the Qt Project for the open source development of Qt!"
        "<p>Thanks to Dirk Clemens for his excellent <a href='http://wit.wiimm.de'>WIT</a> (Wiimms ISO Tools)!"
        "<p>" + QCoreApplication::applicationName() + " is licensed under the GNU General Public License v3 (<a href='http://www.gnu.org/licenses/gpl-3.0.txt'>GPLv3</a>)."
        "<p><i>Dedicated in loving memory of my father G&uuml;nter Heitkamp (28.07.1935 - 06.10.2009)</i>";

    QMessageBox messageBox;
    messageBox.setIconPixmap(QPixmap(":/images/appicon.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    messageBox.setWindowTitle(tr("About ") + QCoreApplication::applicationName());
    messageBox.setText(message);

    messageBox.addButton(QMessageBox::Cancel);
    messageBox.addButton(QMessageBox::Ok);
    messageBox.button(QMessageBox::Cancel)->setText("About &Qt");
    messageBox.setDefaultButton(QMessageBox::Ok);
    connect(messageBox.button(QMessageBox::Cancel), SIGNAL(clicked()), qApp, SLOT(aboutQt()));

    QGridLayout* gridLayout = qobject_cast<QGridLayout*>(messageBox.layout());
    QSpacerItem* horizontalSpacer = new QSpacerItem((qApp->fontMetrics().ascent() - qApp->fontMetrics().descent()) * 75, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(horizontalSpacer, gridLayout->rowCount(), 0, 1, gridLayout->columnCount());

    messageBox.exec();
}

void WiiBaFu::keyPressEvent(QKeyEvent *keyEvent) {
    if (keyEvent->modifiers() == Qt::ControlModifier) {
        switch (keyEvent->key()) {
            case Qt::Key_1: setView(0); break;
            case Qt::Key_2: setView(1); break;
            case Qt::Key_3: setView(2); break;
            case Qt::Key_4: setView(3); break;
            case Qt::Key_5: setView(4); break;

            case Qt::Key_Q: close();    break;
        }
    }

    QMainWindow::keyPressEvent(keyEvent);
}

bool WiiBaFu::event(QEvent *event) {
    if (event->type() == QEvent::StatusTip) {
        return false;
    }
    else {
        return QMainWindow::event(event);
    }
}

WiiBaFu::~WiiBaFu() {
    saveMainWindowGeometry();
    saveGameListHeaderStates();

    logFile.close();

    delete wiTools;
    delete common;
    delete settings;
    delete wiibafudialog;
    delete coverViewDialog;
    delete wbfsDialog;
    delete filesListModel;
    delete dvdListModel;
    delete wbfsListModel;
    delete progressBar_Main;
    delete timer;
    delete ui;
}
