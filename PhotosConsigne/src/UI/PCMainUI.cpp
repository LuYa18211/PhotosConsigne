

/**
 * \file PCMainUI.cpp
 * \brief defines PCMainUI
 * \author Florian Lance
 * \date 04/04/2017
 */

// local
#include "PCMainUI.hpp"

// Qt
#include <QCollator>
#include <QMessageBox>
#include <QtGlobal>
#include <QDesktopServices>
#include <QColorDialog>

// std
#include <chrono>

// test
#include "ImageReader.hpp"


using namespace pc;

PCMainUI::PCMainUI(QApplication *parent) : m_ui(new Ui::PhotosConsigneMainW)
{
    Q_UNUSED(parent)
//    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(), end;

    // current version
    QString version = "3.0";

    // use designer ui
    m_ui->setupUi(this);

    // set icon/title
    this->setWindowTitle("PhotosConsigne " + version + " (générateur de PDF à partir de photos)");
    this->setWindowIcon(QIcon(":/images/icon"));

    // disable textes info tab
    m_ui->twGeneralSettings->setTabEnabled(0,false);
    m_ui->twPhotosList->setTabEnabled(1,false);
    m_ui->twPagesList->setTabEnabled(1,false);
    m_ui->tbRight->setItemEnabled(2, false);

    // init text edit widgets
    //  title
    m_titleTEdit = new RichTextEdit();
    m_titleTEdit->set_doc_locker(&m_docLocker);
    m_ui->hlTitleBottom->addWidget(m_titleTEdit);
    m_titleTEdit->setEnabled(false);
    m_titleTEdit->init_as_title();
    m_settings.titleDoc = m_titleTEdit->textEdit();

    //  global consign
    m_globalConsignTEdit = new RichTextEdit();
    m_globalConsignTEdit->set_doc_locker(&m_docLocker);
    m_globalConsignTEdit->init_as_consign();
    m_ui->vlGlobalConsign->addWidget(m_globalConsignTEdit);
    m_settings.globalConsignDoc =  m_globalConsignTEdit->textEdit();

    // init definition
    PaperFormat pf(m_ui->cbDPI->currentText(),m_ui->cbFormat->currentText());
    int currentDPI = m_ui->cbDPI->currentText().toInt();
    m_ui->laDefWxH->setText(QString::number(pf.widthPixels(currentDPI)) + "x" + QString::number(pf.heightPixels(currentDPI)));
    m_ui->laDefTotal->setText("(" + QString::number(pf.widthPixels(currentDPI)*pf.heightPixels(currentDPI)) + " pixels)");

    // init photo display widgets
    QHBoxLayout *selectedPhotoLayout = new QHBoxLayout();
    m_ui->wSelectedPhotoW->setLayout(selectedPhotoLayout);
    m_selectedPhotoW = new ImageLabel();
    selectedPhotoLayout->addWidget(m_selectedPhotoW);
    m_ui->twMiddle->setTabEnabled(0, false);
    m_ui->pbAdd->hide();

    // init preview image widget
    m_previewW = new PreviewLabel();
    m_ui->vlImagePreview->addWidget(m_previewW);

    // init workers
    m_displayPhotoWorker = std::make_unique<PhotoDisplayWorker>();
    m_pdfGeneratorWorker = std::make_unique<PDFGeneratorWorker>();

    // connections    
    define_workers_connections();
    define_main_UI_connections();

    // init threads
    m_displayPhotoWorker->moveToThread(&m_displayPhotoWorkerThread);
    m_displayPhotoWorkerThread.start();
    m_pdfGeneratorWorker->moveToThread(&m_pdfGeneratorWorkerThread);
    m_pdfGeneratorWorkerThread.start();
    m_ui->tbRight->setCurrentIndex(1);

    update_settings();    
}

PCMainUI::~PCMainUI()
{
    emit kill_signal();

    m_displayPhotoWorkerThread.quit();
    m_displayPhotoWorkerThread.wait();

    m_pdfGeneratorWorkerThread.quit();
    m_pdfGeneratorWorkerThread.wait();

    m_individualConsignsTEdit.clear();
    m_individualConsignsW.clear();

    if(m_wHelp != nullptr){
        delete m_wHelp;
    }

    if(m_wSupport != nullptr){
        delete m_wSupport;
    }

    delete m_ui;
}


void PCMainUI::set_photos_directory()
{
    QString previousDirectory = m_settings.photosDirectoryPath;
    m_settings.photosDirectoryPath = QFileDialog::getExistingDirectory(this, "Sélection du répertoire d'images", QDir::homePath());

    // no directory selected
    if(m_settings.photosDirectoryPath.size() == 0 ){
        if(previousDirectory.size() != 0)
            m_settings.photosDirectoryPath = previousDirectory;
        return;
    }

    // retrieve dir
    QDir dir(m_settings.photosDirectoryPath);
    dir.setSorting(QDir::NoSort);  // will sort manually with std::sort
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.jfif" << "*.jpe" << "*.tif" << "*.gif" << "*.bmp" << "*.pdm" << "*.ppm" << "*.xdm" << "*.xpm");

    // retrieve photos name list
    QStringList fileList = dir.entryList();

    // sort list with std
    QCollator collator;
    collator.setNumericMode(true);
    std::sort( fileList.begin(), fileList.end(),
        [&collator](const QString &file1, const QString &file2){
            return collator.compare(file1, file2) < 0;
        }
    );

    // no photo in the directory
    if(fileList.size() == 0){
        if(previousDirectory.size() != 0){
            m_settings.photosDirectoryPath = previousDirectory;
        }

        QMessageBox::warning(this, tr("Avertissement"), tr("Aucune image (jpg,png) n'a pu être trouvée dans ce répertoire, veuillez en selectionner un autre.\n"),QMessageBox::Ok);
        return;
    }

    QString buttonText = m_settings.photosDirectoryPath;
    if(buttonText.size() > 30){

        int indexMid = buttonText.indexOf('/', buttonText.size()/2);
        if(indexMid > 0){
            buttonText = buttonText.insert(indexMid,'\n');
        }
    }
    QString styleSheet ="QPushButton{color :rgb(0,106,255); border-style: outset;font-style: normal;font-size: 8pt;background-color:  rgb(220,220,220);} \
            QPushButton:pressed {background-color: white;background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #f6f7fa, stop: 1 #dadbde);} \
           QPushButton:hover:!pressed {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #dadbde, stop: 1 #f6f7fa);}";
    m_ui->pbChooseDirectory->setMinimumWidth(0);
    m_ui->pbChooseDirectory->setStyleSheet(styleSheet);
    m_ui->pbChooseDirectory->setText(buttonText);

    if(fileList.size() > 0){
        m_ui->lwPhotosList->setCurrentRow(0);
    }

    m_ui->lwPagesList->clear();
    m_ui->lwPagesList->setEnabled(false);

    m_ui->lwPhotosList->clear();
    m_ui->lwPhotosList->setEnabled(false);

    m_ui->twPhotosList->setTabText(1, QString::number(0));
    m_ui->twPagesList->setTabText(1, QString::number(0));

    m_ui->pbChooseDirectory->setEnabled(false);

    emit send_photos_dir_signal(m_settings.photosDirectoryPath, fileList);

    // test
    // ##########################################################################

//        ImageReader5 reader;

////        connect(&reader, &ImageReader5::set_progress_bar_state_signal, m_ui->progressBarLoading, &QProgressBar::setValue);
////        connect(&reader, &ImageReader5::set_progress_bar_state_signal(), )
//    //    QFuture<QImage> future = reader.read(INPUT);

//    //    QFutureWatcher<QImage> *watcher = new QFutureWatcher<QImage>();

//    //    QObject::connect(watcher, &QFutureWatcher<QImage>::finished,
//    //            [=]() {
//    //        setImage(future.result());
//    //    });

//    //    watcher->setFuture(future);

//        QFuture<SPhotos> future = reader.read_photos(m_settings.photosDirectoryPath, fileList);


//        QFutureWatcher<SPhotos> *watcher = new QFutureWatcher<SPhotos>();
//        QObject::connect(watcher, &QFutureWatcher<QImage>::finished,[=]() {

//            qDebug() << "image finished";
////            m_ui->lwPhotosList->addItem(QString::number(m_ui->lwPhotosList->count()+1) + ". " + "test");
////            m_ui->twPhotosList->setTabText(1, QString::number(m_ui->lwPhotosList->count()+1));

////            QBrush brush = m_ui->lwPhotosList->item(m_ui->lwPhotosList->count()-1)->foreground();
////            brush.setColor(QRgb(qRgb(0,106,255)));
////            m_ui->lwPhotosList->item(m_ui->lwPhotosList->count()-1)->setForeground(brush);

////            if(m_ui->lwPhotosList->count() == 1)
////                m_ui->lwPhotosList->setCurrentRow(0);

////            setImage(future.result());
//        });
//        watcher->setFuture(future);
}

void PCMainUI::end_loading_photos(SPhotos photos)
{
    m_ui->lwPhotosList->setEnabled(true);
    m_ui->pbChooseDirectory->setEnabled(true);
    m_ui->lwPagesList->setEnabled(true);
    m_ui->tbRight->setItemEnabled(2, true);
    m_settings.photosLoaded = photos;

    update_photos_number(true);
    update_settings();

    m_ui->laLoadingText->setText("Photos chargées.");
}

void PCMainUI::update_photos_number(bool reset)
{
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

    // build valid photos array
    m_settings.nbPhotosPageH = m_ui->sbNbPhotosH->value();
    m_settings.nbPhotosPageV = m_ui->sbNbPhotosV->value();
    m_settings.photosValided->clear();
    m_settings.photosValided->reserve(m_settings.photosLoaded->size());
    for(int ii = 0; ii < m_settings.photosLoaded->size(); ++ii){

        if(!m_settings.photosLoaded.get()->at(ii)->isRemoved){
            m_settings.photosValided->push_back(m_settings.photosLoaded.get()->at(ii));
        }
    }

    // display pages number
    auto nbTotalPhotos = m_settings.photosValided->size();
    m_settings.nbPhotosPerPage = m_settings.nbPhotosPageH * m_settings.nbPhotosPageV;    
    m_ui->twPhotosList->setTabText(1, QString::number(nbTotalPhotos));


    auto init_individual_consign_ui = [&](int idConsign){


//        qDebug()<< "update_photos_number before w 1: " << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now()-start).count() << " ms";

        std::shared_ptr<QWidget> SQWidget = std::make_shared<QWidget>();
        std::shared_ptr<RichTextEdit> SRichTextEdit = std::make_shared<RichTextEdit>();
        SRichTextEdit->set_doc_locker(&m_docLocker);
        SRichTextEdit->init_as_individual_consign();

//        qDebug()<< "update_photos_number before w 2: " << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now()-start).count() << " ms";
        m_individualConsignsW.push_back(SQWidget);
        m_individualConsignsTEdit.push_back(SRichTextEdit);

        Ui::IndividualConsign ui;
        ui.setupUi(SQWidget.get());
        ui.vlIndividualConsign->addWidget(SRichTextEdit.get());
        associate_buttons({ui.pbConsignBottom, ui.pbConsignLeft, ui.pbConsignRight, ui.pbConsignTop});
        associate_buttons({ui.pbImageAligmentRight, ui.pbImageAligmentLeft, ui.pbImageAligmentHMiddle});
        associate_buttons({ui.pbImageAligmentBottom, ui.pbImageAligmentVMiddle, ui.pbImageAligmentTop});
        update_settings_buttons({ui.pbConsignBottom, ui.pbConsignLeft, ui.pbConsignRight, ui.pbConsignTop,
                                 ui.pbImageAligmentRight, ui.pbImageAligmentLeft, ui.pbImageAligmentHMiddle,
                                 ui.pbImageAligmentBottom, ui.pbImageAligmentVMiddle, ui.pbImageAligmentTop}, true);
        update_settings_sliders({ui.hsRatioPC}, true);
        update_settings_double_spinboxes({ui.dsbRatioPC}, true);
        update_settings_checkboxes({ui.cbEnableIndividualConsign});
        connect(SRichTextEdit.get()->textEdit(), &TextEdit::textChanged, this, &PCMainUI::update_settings);


//        qDebug()<< "update_photos_number before w 3: " << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now()-start).count() << " ms";
        connect(ui.cbWriteOnPhoto, &QCheckBox::clicked, this, [=](bool checked){

            if(checked){
                m_settings.previousIndividualConsignPositionFromPhotos[idConsign] = (!ui.pbConsignBottom->isEnabled() ? Position::bottom :
                                                                                    (!ui.pbConsignLeft->isEnabled()   ? Position::left :
                                                                                    (!ui.pbConsignRight->isEnabled()  ? Position::right : Position::top)));
            }

            ui.hsRatioPC->setEnabled(!checked);
            ui.dsbRatioPC->setEnabled(!checked);
            ui.pbConsignBottom->setEnabled(!checked);
            ui.pbConsignLeft->setEnabled(!checked);
            ui.pbConsignRight->setEnabled(!checked);
            ui.pbConsignTop->setEnabled(!checked);

            if(!checked){
                switch(m_settings.previousIndividualConsignPositionFromPhotos[idConsign]){
                    case Position::bottom:
                        ui.pbConsignBottom->setEnabled(false);
                    break;
                    case Position::left:
                        ui.pbConsignLeft->setEnabled(false);
                    break;
                    case Position::right:
                        ui.pbConsignRight->setEnabled(false);
                    break;
                    case Position::top:
                         ui.pbConsignTop->setEnabled(false);
                    break;
                }
            }

            update_settings();
//            qDebug()<< "update_photos_number before w 4: " << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now()-start).count() << " ms";
        });
        associate_double_spinbox_with_slider(ui.dsbRatioPC, ui.hsRatioPC);
    };

    qreal currentState = 750.;
    if(reset){
        m_individualConsignsTEdit.clear();
        m_individualConsignsW.clear();
        m_settings.previousIndividualConsignPositionFromPhotos.clear();

        m_individualConsignsTEdit.reserve(nbTotalPhotos);
        m_individualConsignsW.reserve(nbTotalPhotos);
        m_settings.previousIndividualConsignPositionFromPhotos.reserve(nbTotalPhotos);

        qreal offset = 250. / nbTotalPhotos;
        for(int ii = 0; ii < nbTotalPhotos; ++ii){
            m_settings.previousIndividualConsignPositionFromPhotos.push_back(Position::top);

            m_ui->laLoadingText->setText("Consigne individuelle n°" + QString::number( m_settings.previousIndividualConsignPositionFromPhotos.size()));
            init_individual_consign_ui(ii);
            currentState += offset;
            m_ui->progressBarLoading->setValue(currentState);
        }
    }
    else{

        if(nbTotalPhotos < m_individualConsignsW.size()){
            for(int ii = 0; ii < m_individualConsignsW.size()-nbTotalPhotos; ++ii){
                m_individualConsignsTEdit.removeLast();
                m_individualConsignsW.removeLast();
                m_settings.previousIndividualConsignPositionFromPhotos.removeLast();
            }
        }
        else if(nbTotalPhotos > m_individualConsignsW.size()){
            m_individualConsignsTEdit.reserve(nbTotalPhotos);
            m_individualConsignsW.reserve(nbTotalPhotos);
            m_settings.previousIndividualConsignPositionFromPhotos.reserve(nbTotalPhotos);

            qreal offset = 250. / (nbTotalPhotos-m_individualConsignsW.size());
            for(int ii = 0; ii < nbTotalPhotos-m_individualConsignsW.size(); ++ii){
                m_settings.previousIndividualConsignPositionFromPhotos.push_back(Position::top);

                m_ui->laLoadingText->setText("Consigne individuelle n°" + QString::number( m_settings.previousIndividualConsignPositionFromPhotos.size()));
                init_individual_consign_ui(ii);
                currentState += offset;
                m_ui->progressBarLoading->setValue(currentState);
            }
        }
    }

    if(m_settings.currentPCDisplayed > nbTotalPhotos){
        m_settings.currentPCDisplayed = nbTotalPhotos-1;
    }

    // update number of individual text edit widgets
    int currentIndex = m_ui->tbRight->currentIndex();
    if(m_ui->tbRight->count() > 2){
        m_ui->tbRight->removeItem(2);
    }


    if(m_settings.currentPCDisplayed > 0){
        m_ui->tbRight->addItem(m_individualConsignsW[m_settings.currentPCDisplayed].get(), "Sélection ensemble consigne-photo n°" + QString::number(m_settings.currentPCDisplayed));
        m_ui->tbRight->setCurrentIndex(currentIndex);
    }

    m_ui->progressBarLoading->setValue(1000);

//    qDebug()<< "update_photos_number end: " << std::chrono::duration_cast<std::chrono::milliseconds>
//                             (std::chrono::system_clock::now()-start).count() << " ms";
}

void PCMainUI::reset_pc_pages()
{
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

    if(m_settings.nbPhotosPerPage == 0){
        return;
    }

    m_ui->lwPagesList->blockSignals(true);
    m_ui->twMiddle->blockSignals(true);

    // compute pages number
    auto nbPages                = m_settings.photosValided->size() / m_settings.nbPhotosPerPage;
    m_settings.lastPagePhotosNb = m_settings.photosValided->size() % m_settings.nbPhotosPerPage;
    if( m_settings.lastPagePhotosNb != 0)
        ++nbPages;

    if(m_settings.lastPagePhotosNb == 0)
        m_settings.lastPagePhotosNb = m_settings.nbPhotosPerPage;

    // enable preview tab
    m_ui->twMiddle->setTabEnabled(1, nbPages != 0);

    // reset pages
    m_settings.currentPageId = m_ui->lwPagesList->currentRow();
    m_ui->lwPagesList->clear();
    m_pcPages.pages.clear();
    m_pcPages.pages.reserve(nbPages);

    // create new pages
    int currentPhotoId = 0;
    for(int ii = 0; ii < nbPages; ++ii){

        // define current nb of photos for this page
        int nbPhotosPage = m_settings.nbPhotosPerPage;
        if(m_settings.lastPagePhotosNb > 0 && (ii == nbPages -1)){
            nbPhotosPage = m_settings.lastPagePhotosNb;
        }

        SPCPage pcPage = std::make_shared<PCPage>(PCPage());
        // misc
        pcPage->id = ii;
        pcPage->nbPhotosH = m_settings.nbPhotosPageH;
        pcPage->nbPhotosV = m_settings.nbPhotosPageV;

        // sets
        pcPage->sets.reserve(nbPhotosPage);
        for(int jj = 0; jj < nbPhotosPage; ++jj){

            SPCSet set = std::make_shared<PCSet>(PCSet());
            set->id = jj;
            set->totalId = currentPhotoId;
            set->photo = m_settings.photosValided->at(currentPhotoId++);
            set->consign = std::make_shared<Consign>(Consign());
            pcPage->sets.push_back(set);
        }

        m_pcPages.pages.push_back(pcPage);        
        m_ui->lwPagesList->addItem(QString::number(ii+1) + ". " + QString::number(nbPhotosPage) + ((nbPhotosPage > 1 ) ? " images." : " image."));

        QBrush brush = m_ui->lwPagesList->item(m_ui->lwPagesList->count()-1)->foreground();
        brush.setColor(QRgb(qRgb(0,106,255)));
        m_ui->lwPagesList->item(m_ui->lwPagesList->count()-1)->setForeground(brush);
    }

    // update current row
    if(m_settings.currentPageId != -1){
        if(m_settings.currentPageId >= nbPages){
            m_settings.currentPageId = nbPages-1;
        }
        m_ui->lwPagesList->setCurrentRow(m_settings.currentPageId);
    }else{
        m_ui->lwPagesList->setCurrentRow(0);
        m_settings.currentPageId = 0;
    }

    // update pages number on the tab text
    m_ui->twPagesList->setTabText(1, QString::number(m_pcPages.pages.size()));
    m_ui->lwPagesList->blockSignals(false);
    m_ui->twMiddle->blockSignals(false);


//    qDebug()<< "reset_pc_pages end: " << std::chrono::duration_cast<std::chrono::milliseconds>
//                             (std::chrono::system_clock::now()-start).count() << " ms";
}

void PCMainUI::update_photo_to_display(SPhoto photo)
{
//    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    m_selectedPhotoW->set_image(QImage(photo->pathPhoto).transformed(QTransform().rotate(photo->rotation)));
    m_selectedPhotoW->update();

    if(photo->isRemoved){
        m_ui->pbRemove->hide();
        m_ui->pbAdd->show();
    }
    else{
        m_ui->pbAdd->hide();
        m_ui->pbRemove->show();
    }

//    qDebug()<< "update_photo_to_display end: " << std::chrono::duration_cast<std::chrono::milliseconds>
    //                             (std::chrono::system_clock::now()-start).count() << " ms";
}

void PCMainUI::define_workers_connections()
{
    // ui -> pdf generator worker
    connect(this, &PCMainUI::kill_signal,                       m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::kill);
    connect(this, &PCMainUI::start_preview_generation_signal,   m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::generate_preview);
    connect(this, &PCMainUI::start_PDF_generation_signal,       m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::generate_PDF);

    // pdf generator worker -> ui
    connect(m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::end_preview_signal,            this, [=](QImage previewImage){
        m_previewW->set_image(previewImage);
        m_previewW->update();
        m_ui->twMiddle->setCurrentIndex(1);
        m_ui->pbSavePDF->setEnabled(true);

        m_previewLocker.lockForWrite();
        m_isPreviewComputing = false;
        bool generatePreviewAgain = m_generatePreviewAgain;
        m_generatePreviewAgain = false;
        m_previewLocker.unlock();

        if(generatePreviewAgain){
            update_settings();
        }

    });
    connect(m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::end_generation_signal,         this, [=](bool success){

        m_ui->pbSavePDF->setEnabled(true);
        if(success){
            m_ui->pbOpenPDF->setEnabled(true);
            m_ui->laLoadingText->setText("PDF généré avec succès\n" + m_pcPages.pdfFileName);
        }else{
            m_ui->laLoadingText->setText("Echec génération du PDF");
        }
    });
    connect(m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::set_progress_bar_state_signal, m_ui->progressBarLoading, &QProgressBar::setValue);
    connect(m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::set_progress_bar_text_signal,  m_ui->laLoadingText, &QLabel::setText);
    connect(m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::abort_pdf_signal,              this, [=](QString pathFile){

        QMessageBox::warning(this, tr("Avertissement"), tr("Le fichier PDF ") + pathFile + tr(" n'a pu être écrit, il se peut que celui-ci soit en cours d'utilisation par un autre logiciel."),QMessageBox::Ok);
        m_ui->laLoadingText->setText("Echec génération du PDF");
        m_ui->pbSavePDF->setEnabled(true);
    });

    // pdf worker -> preview worker
    connect(m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::current_pc_selected_signal, this, [=](QRectF pcRectRelative, int totalIdPC){

        if(totalIdPC == -1){
            m_ui->tbRight->setCurrentIndex(0);
        }else{
            m_ui->tbRight->removeItem(2);
            m_ui->tbRight->addItem(m_individualConsignsW[totalIdPC].get(), "Sélection ensemble consigne-photo n°" + QString::number(totalIdPC));
            m_settings.currentPCDisplayed = totalIdPC;
            m_ui->tbRight->setCurrentIndex(2);
        }

        m_previewW->draw_current_pc_rect(totalIdPC, pcRectRelative);
        m_previewW->update();
    });

    // preview label -> ui
    connect(m_previewW, &PreviewLabel::double_click_on_photo_signal, this, [&](int idTotalPhoto){

        int offset = 0;
        QVector<int> idPhotos;
        for(int ii = 0; ii < m_settings.photosLoaded->size(); ++ii){
            if(m_settings.photosLoaded.get()->at(ii)->isRemoved){
                ++offset;
            }else{
                idPhotos.push_back(offset++);
            }
        }
        m_ui->lwPhotosList->setCurrentRow(idPhotos[idTotalPhoto]);
        m_ui->twMiddle->setCurrentIndex(0);
    });

    // preview label -> pdf worker
    connect(m_previewW, &PreviewLabel::click_on_page_signal, m_pdfGeneratorWorker.get(), &PDFGeneratorWorker::update_PC_selection);

    // ui -> photo display worker
    connect(this, &PCMainUI::kill_signal,            m_displayPhotoWorker.get(), &PhotoDisplayWorker::kill);
    connect(this, &PCMainUI::send_photos_dir_signal, m_displayPhotoWorker.get(), &PhotoDisplayWorker::load_photos);
    // photo display worker -> ui
    connect(m_displayPhotoWorker.get(), &PhotoDisplayWorker::set_progress_bar_state_signal, m_ui->progressBarLoading, &QProgressBar::setValue);
    connect(m_displayPhotoWorker.get(), &PhotoDisplayWorker::set_progress_bar_text_signal,  m_ui->laLoadingText, &QLabel::setText);
    connect(m_displayPhotoWorker.get(), &PhotoDisplayWorker::end_loading_photos_signal,     this, &PCMainUI::end_loading_photos);
    connect(m_displayPhotoWorker.get(), &PhotoDisplayWorker::photo_loaded_signal,           this, [&](QString image){

        m_ui->lwPhotosList->addItem(QString::number(m_ui->lwPhotosList->count()+1) + ". " + image);
        m_ui->twPhotosList->setTabText(1, QString::number(m_ui->lwPhotosList->count()+1));

        QBrush brush = m_ui->lwPhotosList->item(m_ui->lwPhotosList->count()-1)->foreground();
        brush.setColor(QRgb(qRgb(0,106,255)));
        m_ui->lwPhotosList->item(m_ui->lwPhotosList->count()-1)->setForeground(brush);

        if(m_ui->lwPhotosList->count() == 1)
            m_ui->lwPhotosList->setCurrentRow(0);
    });
    connect(m_displayPhotoWorker.get(), &PhotoDisplayWorker::end_loading_photos_signal, [&](SPhotos photos){
        m_settings.photosLoaded = photos;

        if( m_settings.photosLoaded->size() > 0){
            m_ui->twMiddle->setTabEnabled(0, true);
            m_ui->twMiddle->setCurrentIndex(0);
            m_settings.currentIdImageDisplayed = 0;
            update_photo_to_display(photos->at(0));
        }
    });
}

void PCMainUI::define_main_UI_connections()
{
    // timer
    connect(&m_zonesTimer, &QTimer::timeout, this, [=]{
        m_zonesTimer.stop();
        update_settings();
    });


    // page color
    QPixmap pixColor(16, 16);
    pixColor.fill(Qt::white);
    QAction *actionPageColor = new QAction(pixColor, tr("Couleur de la page"), this);
    connect(actionPageColor, &QAction::triggered, this, [=]{

        QColor col = QColorDialog::getColor(m_settings.pageColor, this, "Choix de la couleur de page");
        if (!col.isValid())
            return;

        QPixmap pix(16, 16);
        pix.fill(col);
        actionPageColor->setIcon(pix);
        m_settings.pageColor = col;
        update_settings();
    });
    m_ui->tbColorPage->setDefaultAction(actionPageColor);

    // grayscale
    connect(m_ui->cbBAndW, &QCheckBox::clicked, this, [=](bool checked){
        m_settings.grayScale = checked;
        update_settings();
    });

    // # push_button
    connect(m_ui->pbDonate, &QPushButton::clicked, this, [=]{
        if(m_wSupport != nullptr){
            delete m_wSupport;
        }
        m_wSupport = new QWidget();
        m_wSupport->setWindowIcon(QIcon(":/images/icon"));

        Ui::SupportW support;
        support.setupUi(m_wSupport);

        delete support.laPatreon;
        delete support.pbPatreon;
        delete support.hlPatreon;
        connect(support.pbPaypal, &QPushButton::clicked, this, [=]{
            if(!QDesktopServices::openUrl(QUrl("https://www.paypal.me/PhotosConsigne", QUrl::TolerantMode))){
                QMessageBox::warning(this, tr("Avertissement"), tr("Le site internet du tutoriel n'a pu être lancé, aucun logiciel de navigation web n'a été trouvé.' .\n"),QMessageBox::Ok);
            }
        });
        connect(support.pbMail, &QPushButton::clicked, this, [=]{
            if(!QDesktopServices::openUrl(QUrl("mailto:?to=lance.florian@protonmail.com&subject=PhotosConsigne&body=", QUrl::TolerantMode))){
            }
        });
        connect(support.pbReturn, &QPushButton::clicked, this, [=]{
            m_wSupport->hide();
        });

        m_wSupport->show();
    });
    connect(m_ui->pbHelp, &QPushButton::clicked, this, [=]{
        if(m_wHelp != nullptr){
            delete m_wHelp;
        }
        m_wHelp = new QWidget();
        m_wHelp->setWindowIcon(QIcon(":/images/icon"));

        Ui::HelpW help;
        help.setupUi(m_wHelp);

        connect(help.pbFAQ, &QPushButton::clicked, this, [=]{
            if(!QDesktopServices::openUrl(QUrl("https://github.com/FlorianLance/PhotosConsigne/wiki/FAQ:-Foire-aux-questions", QUrl::TolerantMode))){
                QMessageBox::warning(this, tr("Avertissement"), tr("Le site internet du tutoriel n'a pu être lancé, aucun logiciel de navigation web n'a été trouvé.' .\n"),QMessageBox::Ok);
            }
        });
        connect(help.pbVideosTutorials, &QPushButton::clicked, this, [=]{
            if(!QDesktopServices::openUrl(QUrl("https://github.com/FlorianLance/PhotosConsigne/wiki/Tutoriels", QUrl::TolerantMode))){
                QMessageBox::warning(this, tr("Avertissement"), tr("Le site internet du tutoriel n'a pu être lancé, aucun logiciel de navigation web n'a été trouvé.' .\n"),QMessageBox::Ok);
            }
        });

        connect(help.pbReturn, &QPushButton::clicked, this, [=]{
            m_wHelp->hide();
        });

        m_wHelp->show();
    });

    connect(m_ui->pbOpenPDF, &QPushButton::clicked, this, [=]{
        bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(m_pcPages.pdfFileName));
        if(!success)
            QMessageBox::warning(this, tr("Avertissement"), tr("Le PDF n'a pu être lancé.\nVeuillez vous assurez que vous disposez d'un logiciel de lecture de PDF (ex : AdobeReader, SumatraPDF, FoxitReader...) .\n"),QMessageBox::Ok);
    });
    connect(m_ui->pbChooseDirectory, &QPushButton::clicked, this, &PCMainUI::set_photos_directory);
    connect(m_ui->pbSavePDF, &QPushButton::clicked, this, [&]{
        QString filePath = QFileDialog::getSaveFileName(this, "Entrez le nom du fichier PDF", QDir::homePath(), "PDF (*.pdf)");
        if(filePath.size() > 0){
            m_pcPages.pdfFileName = filePath;
            m_ui->pbSavePDF->setEnabled(false);
            m_ui->pbOpenPDF->setEnabled(false);
            emit start_PDF_generation_signal(&m_docLocker, m_settings, m_pcPages);
        }
    });
    connect(m_ui->pbLeft, &QPushButton::clicked, this, [&]{ // previous image
        int &currId = m_settings.currentIdImageDisplayed;
        if(currId == 0){
            currId = m_settings.photosLoaded->size()-1;
        } else{
            currId--;
        }

       update_photo_to_display(m_settings.photosLoaded.get()->at(currId));
       m_ui->lwPhotosList->setCurrentRow(currId);
    });
    connect(m_ui->pbRight, &QPushButton::clicked, this, [&]{ // next image

        int &currId = m_settings.currentIdImageDisplayed;
        if(currId == m_settings.photosLoaded->size()-1){
            currId = 0;
        }else{
            currId++;
        }

        update_photo_to_display(m_settings.photosLoaded.get()->at(currId));
        m_ui->lwPhotosList->setCurrentRow(currId);
    });
    connect(m_ui->pbRotateLeft, &QPushButton::clicked, this, [&] // rotate left photo
    {
        SPhoto photo       = m_settings.photosLoaded.get()->at(m_settings.currentIdImageDisplayed);
        photo->rotation    =(photo->rotation - 90)%360;
        photo->scaledPhoto = photo->scaledPhoto.transformed(QTransform().rotate(-90));
        update_photo_to_display(photo);
    });
    connect(m_ui->pbRotateRight, &QPushButton::clicked, this, [&] // rotate right photo
    {
        SPhoto photo        = m_settings.photosLoaded.get()->at(m_settings.currentIdImageDisplayed);
        photo->rotation     = (photo->rotation + 90)%360;
        photo->scaledPhoto  = photo->scaledPhoto.transformed(QTransform().rotate(90));
        update_photo_to_display(photo);
    });
    connect(m_ui->pbRemove, &QPushButton::clicked, this, [&] // add/remove photo from list
    {
        m_settings.photosLoaded.get()->at(m_settings.currentIdImageDisplayed)->isRemoved = true;
        m_ui->pbRemove->hide();
        m_ui->pbAdd->show();

        QBrush brush = m_ui->lwPhotosList->currentItem()->foreground();
        brush.setColor(QRgb(qRgb(127,180,255)));
        m_ui->lwPhotosList->currentItem()->setForeground(brush);

        qDebug() << "remove 1";
        update_photos_number();
        qDebug() << "remove 2";
        update_settings_with_no_preview();
        qDebug() << "remove 3";
    });
    connect(m_ui->pbDuplicate, &QPushButton::clicked, this, [&]{

        SPhoto duplicatedPhoto = std::make_shared<Photo>(Photo(*(m_settings.photosLoaded.get()->at(m_settings.currentIdImageDisplayed))));
        duplicatedPhoto->isRemoved = false;

        m_settings.photosLoaded->push_back(duplicatedPhoto);

        m_ui->lwPhotosList->blockSignals(true);
        m_ui->lwPhotosList->addItem("(Copie) " + QString::number(m_settings.currentIdImageDisplayed+1) + ". " + duplicatedPhoto->pathPhoto.split("/").last() + "");
        QBrush brush = m_ui->lwPhotosList->currentItem()->foreground();
        brush.setColor(QRgb(qRgb(0,106,255)));
        m_ui->lwPhotosList->item(m_ui->lwPhotosList->count()-1)->setForeground(brush);
        m_ui->lwPhotosList->blockSignals(false);

        update_photos_number();
        update_settings_with_no_preview();
    });

    connect(m_ui->pbAdd, &QPushButton::clicked, this, [&]{// add/remove photo from list
        m_settings.photosLoaded.get()->at(m_settings.currentIdImageDisplayed)->isRemoved = false;
        m_ui->pbRemove->show();
        m_ui->pbAdd->hide();

        QBrush brush = m_ui->lwPhotosList->currentItem()->foreground();
        brush.setColor(QRgb(qRgb(0,106,255)));
        m_ui->lwPhotosList->currentItem()->setForeground(brush);

        update_photos_number();
        update_settings_with_no_preview();
    });
    connect(m_ui->pbOrderMinus, &QPushButton::clicked, this, [&]{

        int row = m_ui->lwPhotosList->currentRow();
        if(row < 0){
            return;
        }

        m_settings.photosLoaded->swap(row,row-1);
        m_ui->lwPagesList->blockSignals(true);
        m_ui->lwPhotosList->insertItem(row-1,m_ui->lwPhotosList->takeItem(row));
        m_ui->lwPhotosList->setCurrentRow(row-1);
        m_ui->lwPagesList->blockSignals(false);

        update_photos_number();
        update_settings_with_no_preview();
    });
    connect(m_ui->pbOrderPlus, &QPushButton::clicked, this, [&]{

        int row = m_ui->lwPhotosList->currentRow();
        if(row < 0){
            return;
        }

        m_settings.photosLoaded.get()->swap(row,row+1);
        m_ui->lwPagesList->blockSignals(true);
        m_ui->lwPhotosList->insertItem(row+1,m_ui->lwPhotosList->takeItem(row));
        m_ui->lwPhotosList->setCurrentRow(row+1);
        m_ui->lwPagesList->blockSignals(false);

        update_photos_number();
        update_settings_with_no_preview();
    });

    // # tab widgets
     connect(m_ui->twMiddle, &QTabWidget::currentChanged, this, [&](int index){
        if(index == 1){
            update_settings();
        }
     });
    // # list widget
    connect(m_ui->lwPhotosList, &QListWidget::currentRowChanged, this, [&](int row){
        if(row == -1 || m_settings.photosLoaded->size() == 0)
            return;

        m_ui->pbOrderMinus->setEnabled(row != 0);
        m_ui->pbOrderPlus->setEnabled(row != m_settings.photosLoaded->size()-1);

        m_settings.currentIdImageDisplayed = row;
        update_photo_to_display(m_settings.photosLoaded.get()->at(row));
        m_ui->twMiddle->setCurrentIndex(0);
    });
    connect(m_ui->lwPhotosList, &QListWidget::clicked, this, [&](QModelIndex index){
        if(index.row() == -1 || m_settings.photosLoaded->size() == 0)
            return;

        m_ui->pbOrderMinus->setEnabled(index.row() != 0);
        m_ui->pbOrderPlus->setEnabled(index.row() != m_settings.photosLoaded->size()-1);

        m_settings.currentIdImageDisplayed = index.row();
        update_photo_to_display(m_settings.photosLoaded.get()->at(index.row()));
        m_ui->twMiddle->setCurrentIndex(0);
    });
    connect(m_ui->lwPagesList, &QListWidget::currentRowChanged, this, [&]
    {
        m_previewW->draw_current_pc_rect(-1, QRectF()); // remove current rect

        if(m_ui->lwPagesList->currentRow() != -1){
            m_settings.currentPageId = m_ui->lwPagesList->currentRow();
            update_settings();
        }

    });
    connect(m_ui->lwPagesList, &QListWidget::clicked, this, [&]
    {
        if(m_ui->lwPagesList->currentRow() != -1){
            m_settings.currentPageId = m_ui->lwPagesList->currentRow();
            update_settings();
        }
    });
    // # associate sliders with spin boxes
    associate_double_spinbox_with_slider(m_ui->dsbGlobalRatioPC, m_ui->hsGlobalRatioPC);
    associate_double_spinbox_with_slider(m_ui->dsbRatioTitle, m_ui->hsRatioTitle);
    associate_double_spinbox_with_slider(m_ui->dsbLeftMargins, m_ui->hsLeftMargins, m_ui->dsbRightMargins, m_ui->hsRightMargins);
    associate_double_spinbox_with_slider(m_ui->dsbTopMargins, m_ui->hsTopMargins, m_ui->dsbBottomMargins, m_ui->hsBottomMargins);
    associate_double_spinbox_with_slider(m_ui->dsbHorizontalMargins, m_ui->hsHorizontalMargins);
    associate_double_spinbox_with_slider(m_ui->dsbVerticalMargins, m_ui->hsVerticalMargins);

    // # associate buttons
    associate_buttons({m_ui->pbGlobalConsignBottom,m_ui->pbGlobalConsignLeft,m_ui->pbGlobalConsignRight,m_ui->pbGlobalConsignTop});
    associate_buttons({m_ui->pbGlobalImageAligmentLeft,m_ui->pbGlobalImageAligmentRight, m_ui->pbGlobalImageAligmentHMiddle});
    associate_buttons({m_ui->pbGlobalImageAligmentVMiddle,m_ui->pbGlobalImageAligmentTop,m_ui->pbGlobalImageAligmentBottom});
    associate_buttons({m_ui->pbLandscape,m_ui->pbPortrait});

    // # associate checkboxes with UI
    checkbox_enable_UI(m_ui->cbAddExteriorMargins, {m_ui->hsLeftMargins, m_ui->hsRightMargins, m_ui->hsTopMargins, m_ui->hsBottomMargins,
                       m_ui->dsbLeftMargins, m_ui->dsbRightMargins, m_ui->dsbTopMargins, m_ui->dsbBottomMargins});
    checkbox_enable_UI(m_ui->cbAddInteriorMargins, {m_ui->hsHorizontalMargins,m_ui->hsVerticalMargins,m_ui->dsbHorizontalMargins,m_ui->dsbVerticalMargins});
    checkbox_enable_UI(m_ui->cbAddTitle, {m_ui->cbAllPagesTitle, m_ui->rbBottomTitle, m_ui->rbTopTitle, m_ui->rbWriteOnPCTitle, m_ui->hsRatioTitle, m_ui->dsbRatioTitle, m_titleTEdit});
    connect(m_ui->cbAddTitle, &QCheckBox::clicked, [&](bool checked){
        m_ui->tbRight->setCurrentIndex(checked ? 0 : 1);
    });

    /// # udpate settings
    update_settings_buttons({m_ui->pbGlobalConsignBottom,m_ui->pbGlobalConsignLeft,m_ui->pbGlobalConsignRight,m_ui->pbGlobalConsignTop,
                            m_ui->pbGlobalImageAligmentLeft,m_ui->pbGlobalImageAligmentRight, m_ui->pbGlobalImageAligmentHMiddle,
                            m_ui->pbGlobalImageAligmentVMiddle,m_ui->pbGlobalImageAligmentTop,m_ui->pbGlobalImageAligmentBottom,
                            m_ui->pbLandscape,m_ui->pbPortrait},true);
    update_settings_sliders({m_ui->hsGlobalRatioPC, m_ui->hsRatioTitle, m_ui->hsLeftMargins, m_ui->hsRightMargins, m_ui->hsTopMargins,
                            m_ui->hsBottomMargins, m_ui->hsHorizontalMargins, m_ui->hsVerticalMargins},true);
    update_settings_double_spinboxes({m_ui->dsbGlobalRatioPC, m_ui->dsbRatioTitle, m_ui->dsbLeftMargins, m_ui->dsbRightMargins,
                                     m_ui->dsbTopMargins,m_ui->dsbBottomMargins,m_ui->dsbHorizontalMargins,m_ui->dsbVerticalMargins},true);
    update_settings_checkboxes({m_ui->cbAddExteriorMargins,m_ui->cbAddInteriorMargins,
                               m_ui->cbAddTitle, m_ui->cbAllPagesTitle},true);
    update_settings_checkboxes({m_ui->cbCutLines, m_ui->cbSaveOnlyCurrentPage});

    connect(m_ui->cbWriteOnPhoto, &QCheckBox::clicked, this, [=](bool checked){

        if(checked){
            m_settings.previousGlobalConsignPositionFromPhotos =  m_settings.globalConsignPositionFromPhotos;
        }
        m_ui->pbGlobalConsignBottom->setEnabled(!checked);
        m_ui->pbGlobalConsignLeft->setEnabled(!checked);
        m_ui->pbGlobalConsignRight->setEnabled(!checked);
        m_ui->pbGlobalConsignTop->setEnabled(!checked);
        m_ui->dsbGlobalRatioPC->setEnabled(!checked);
        m_ui->hsGlobalRatioPC->setEnabled(!checked);

        if(!checked){
            switch(m_settings.previousGlobalConsignPositionFromPhotos){
                case Position::bottom:
                    m_ui->pbGlobalConsignBottom->setEnabled(false);
                break;
                case Position::left:
                    m_ui->pbGlobalConsignLeft->setEnabled(false);
                break;
                case Position::right:
                    m_ui->pbGlobalConsignRight->setEnabled(false);
                break;
                case Position::top:
                    m_ui->pbGlobalConsignTop->setEnabled(false);
                break;
            }
        }

        update_settings();
    });

    update_settings_format_combo_boxes({m_ui->cbDPI, m_ui->cbFormat});
    update_settings_radio_buttons({m_ui->rbWriteOnPCTitle});
    connect(m_ui->rbTopTitle, &QRadioButton::clicked, this, [=]{
       m_ui->hsRatioTitle->setEnabled(true);
       m_ui->dsbRatioTitle->setEnabled(true);
    });
    connect(m_ui->rbBottomTitle, &QRadioButton::clicked, this, [=]{
       m_ui->hsRatioTitle->setEnabled(true);
       m_ui->dsbRatioTitle->setEnabled(true);
    });
    connect(m_ui->rbWriteOnPCTitle, &QRadioButton::clicked, this, [=]{
       m_ui->hsRatioTitle->setEnabled(false);
       m_ui->dsbRatioTitle->setEnabled(false);
    });

    update_settings_radio_buttons({m_ui->rbBottomTitle, m_ui->rbTopTitle},true);

    connect(m_ui->sbNbPhotosH, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{
        m_zonesTimer.start(1000);
        update_photos_number();
        update_settings();
    });
    connect(m_ui->sbNbPhotosV, QOverload<int>::of(&QSpinBox::valueChanged), this, [=]{
        m_zonesTimer.start(1000);
        update_photos_number();
        update_settings();
    });
    // # list widgets
    connect(m_titleTEdit->textEdit(), &TextEdit::textChanged, this, &PCMainUI::update_settings);
    connect(m_globalConsignTEdit->textEdit(), &TextEdit::textChanged, this, &PCMainUI::update_settings);

}

void PCMainUI::update_settings()
{
    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    auto nbPhotosValid = m_settings.photosValided->size();
    int currentTabId = m_ui->twMiddle->currentIndex();

    // reset all pc pages
    reset_pc_pages();

    // image alignment
    int globalPhotosAlignment = 0;
    if(!m_ui->pbGlobalImageAligmentHMiddle->isEnabled()){ // horizontal center
        globalPhotosAlignment = globalPhotosAlignment |  Qt::AlignHCenter;
    }else if(!m_ui->pbGlobalImageAligmentRight->isEnabled()){ // horizontal right
        globalPhotosAlignment = globalPhotosAlignment |  Qt::AlignRight;
    }else{ // horizontal left
        globalPhotosAlignment = globalPhotosAlignment |  Qt::AlignLeft;
    }

    if(!m_ui->pbGlobalImageAligmentVMiddle->isEnabled()){// vertical center
        globalPhotosAlignment = globalPhotosAlignment | Qt::AlignVCenter;
    }else if(!m_ui->pbGlobalImageAligmentTop->isEnabled()){ // vertical top
        globalPhotosAlignment = globalPhotosAlignment | Qt::AlignTop;
    }else{
        globalPhotosAlignment = globalPhotosAlignment | Qt::AlignBottom;
    }

    m_settings.saveOnlyCurrentPage   = m_ui->cbSaveOnlyCurrentPage->isChecked();
    m_settings.globalPCRatio         = m_ui->dsbGlobalRatioPC->value();
    m_settings.globalPhotosAlignment = globalPhotosAlignment;
    m_settings.globalOrientation     = m_ui->pbPortrait->isEnabled() ? PageOrientation::landScape : PageOrientation::portrait;
    m_settings.globalDisplayCutLines = m_ui->cbCutLines->isChecked();
    m_pcPages.paperFormat            = PaperFormat(m_ui->cbDPI->currentText(), m_ui->cbFormat->currentText());

    // titles
    m_settings.titleAdded = m_ui->cbAddTitle->isChecked();
    m_settings.titleOnAllPages = m_ui->cbAllPagesTitle->isChecked();
    m_settings.globalRatioTitle = m_ui->dsbRatioTitle->value();
    m_settings.globalTitlePositionFromPC = (m_ui->rbTopTitle->isChecked()) ? Position::top : (m_ui->rbBottomTitle->isChecked() ? Position::bottom : Position::on);
    m_settings.titleDoc = m_titleTEdit->textEdit();

    // margins
    pc::RatioMargins &margins =  m_settings.globalMargins;
    margins.exteriorMarginsEnabled = m_ui->cbAddExteriorMargins->isChecked();
    margins.interiorMarginsEnabled = m_ui->cbAddInteriorMargins->isChecked();
    margins.left = m_ui->dsbLeftMargins->value();
    margins.right = m_ui->dsbRightMargins->value();
    margins.top = m_ui->dsbTopMargins->value();
    margins.bottom = m_ui->dsbBottomMargins->value();
    margins.interWidth = m_ui->dsbHorizontalMargins->value();
    margins.interHeight = m_ui->dsbVerticalMargins->value();

    m_settings.globalConsignPositionFromPhotos = (!m_ui->pbGlobalConsignTop->isEnabled())    ? Position::top :
                                                ((!m_ui->pbGlobalConsignBottom->isEnabled()) ? Position::bottom :
                                                ((!m_ui->pbGlobalConsignLeft->isEnabled())   ? Position::left : Position::right));

    // consigns
    m_settings.globalConsignOnPhoto = m_ui->cbWriteOnPhoto->isChecked();
    m_settings.globalConsignDoc     = m_globalConsignTEdit->textEdit();

    m_settings.consignsDoc.clear();
    m_settings.consignsDoc.reserve(nbPhotosValid);

    for(const auto &richText : m_individualConsignsTEdit){
        m_settings.consignsDoc.push_back(richText->textEdit());
    }

    int currPCId = 0;
    for(auto &&pcPage : m_pcPages.pages){

        // misc
        pcPage->margins         = margins;
        pcPage->displayCutLines = m_settings.globalDisplayCutLines;

        // title
        if(m_settings.titleAdded && (pcPage->id == 0 || m_settings.titleOnAllPages)){
            if(pcPage->title == nullptr){
                pcPage->title = std::make_shared<Title>(Title());
            }
            pcPage->title->doc     = m_settings.titleDoc->document();
            pcPage->titlePositionFromPC = m_settings.globalTitlePositionFromPC;
            pcPage->ratioWithTitle      = m_settings.globalRatioTitle;
        }else{
            pcPage->title = nullptr;
        }

        // sets PC
        for(auto &&pc : pcPage->sets){

            auto wC = m_individualConsignsW[currPCId];

            bool individual = wC->findChild<QCheckBox*>("cbEnableIndividualConsign")->isChecked();// m_individualConsignsTEdit[currPCId]->textEdit()->document()->toPlainText().size() > 0;
            if(!individual){
                pc->consignPositionFromPhoto = m_settings.globalConsignPositionFromPhotos;
                pc->ratio = m_settings.globalPCRatio;
                pc->consignOnPhoto = m_settings.globalConsignOnPhoto;
                pc->consign->doc = m_settings.globalConsignDoc->document();
                pc->photo->alignment = m_settings.globalPhotosAlignment;
                ++currPCId;
                continue;
            }

            pc->consignPositionFromPhoto = ( (!wC->findChild<QPushButton *>("pbConsignTop")->isEnabled())   ? Position::top    :
                                            ((!wC->findChild<QPushButton *>("pbConsignBottom")->isEnabled())? Position::bottom :
                                            ((!wC->findChild<QPushButton *>("pbConsignLeft")->isEnabled())  ? Position::left   : Position::right)));
            pc->ratio = wC->findChild<QDoubleSpinBox*>("dsbRatioPC")->value();
            pc->consignOnPhoto = wC->findChild<QCheckBox *>("cbWriteOnPhoto")->isChecked();
            pc->consign->doc = m_individualConsignsTEdit[currPCId]->textEdit()->document();

            // image alignment
            int individualPhotosAlignment = 0;
            if(!wC->findChild<QPushButton *>("pbImageAligmentHMiddle")->isEnabled()){ // horizontal center
                individualPhotosAlignment = individualPhotosAlignment |  Qt::AlignHCenter;
            }else if(!wC->findChild<QPushButton *>("pbImageAligmentRight")->isEnabled()){ // horizontal right
                individualPhotosAlignment = individualPhotosAlignment |  Qt::AlignRight;
            }else{ // horizontal left
                individualPhotosAlignment = individualPhotosAlignment |  Qt::AlignLeft;
            }

            if(!wC->findChild<QPushButton *>("pbImageAligmentVMiddle")->isEnabled()){// vertical center
                individualPhotosAlignment = individualPhotosAlignment | Qt::AlignVCenter;
            }else if(!wC->findChild<QPushButton *>("pbImageAligmentTop")->isEnabled()){// vertical top
                individualPhotosAlignment = individualPhotosAlignment | Qt::AlignTop;
            }else{
                individualPhotosAlignment = individualPhotosAlignment | Qt::AlignBottom;
            }
            pc->photo->alignment = individualPhotosAlignment;

            ++currPCId;
        }
    }

    if(currentTabId > 0 ){
        m_ui->twMiddle->blockSignals(true);
        m_ui->twMiddle->setCurrentIndex(currentTabId);
        m_ui->twMiddle->blockSignals(false);
    }

    if(nbPhotosValid == 0)
        return;

    m_previewLocker.lockForWrite();
    bool isPreviewComputing = m_isPreviewComputing;
    m_generatePreviewAgain = isPreviewComputing;
    m_previewLocker.unlock();


    m_settings.displayZones = m_zonesTimer.isActive();
    if(!m_settings.noPreviewGeneration && !isPreviewComputing){
        m_previewLocker.lockForWrite();
        m_isPreviewComputing = true;
        m_previewLocker.unlock();
        emit start_preview_generation_signal(&m_docLocker, m_settings, m_pcPages);
    }
//    qDebug()<< "end update settings: " << std::chrono::duration_cast<std::chrono::milliseconds>
//                             (std::chrono::system_clock::now()-start).count() << " ms";

}




