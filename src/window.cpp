#include "pch.h"

#include "window.h"

#include "printmanager.h"
#include "signalhandler.h"
#include "strings.h"

namespace {

    QString StlModelLibraryPath { "/home/lumen/Volumetric/model-library" };

    QStringList LayerThicknessStringList { "50 µm", "100 µm", "200 µm", };
    int LayerThicknessValues[] { 50, 100, 200 };

    QStringList ExposureScaleFactorStringList { "1×", "2×", "3×", "4×", "5×" };
    int ExposureScaleFactorValues[] { 1, 2, 3, 4, 5 };

    class TabIndex {
    public:
        enum {
            Select,
            Prepare,
            Print,
            Status,
        };
    };

    QString SlicerCommand { "slic3r" };

}

Window::Window(bool fullScreen, bool debuggingPosition, QWidget *parent): QMainWindow(parent) {
    QMargins emptyMargins { };

    move( { 0, debuggingPosition ? 560 : 800 } );
    if ( fullScreen ) {
        showFullScreen( );
    } else {
        setFixedSize( 800, 480 );
    }

    printJob = new PrintJob;

    QObject::connect( g_signalHandler, &SignalHandler::quit, this, &Window::signalHandler_quit );

    //
    // "Select" tab
    //

    selectTab = new SelectTab;
    selectTab->setContentsMargins( emptyMargins );
    selectTab->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    QObject::connect( selectTab, &SelectTab::modelLoadComplete, this, &Window::selectTab_modelLoadComplete );

    //
    // "Print" tab
    //

    sliceProgress  = new QLabel( "Not slicing" );
    renderProgress = new QLabel( "Not rendering" );

    sliceProgressLayout = new QFormLayout;
    sliceProgressLayout->setContentsMargins( emptyMargins );
    sliceProgressLayout->addRow( "Slicer status:", sliceProgress  );
    sliceProgressLayout->addRow( "Render status:", renderProgress );

    sliceProgressContainer = new QWidget;
    sliceProgressContainer->setLayout( sliceProgressLayout );
    sliceProgressContainer->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

    layerThicknessComboBox = new QComboBox;
    layerThicknessComboBox->setEditable( false );
    layerThicknessComboBox->setMaxVisibleItems( LayerThicknessStringList.count( ) );
    layerThicknessComboBox->addItems( LayerThicknessStringList );
    layerThicknessComboBox->setCurrentIndex( 1 );
    printJob->layerThickness = 100;
    QObject::connect( layerThicknessComboBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &Window::layerThicknessComboBox_currentIndexChanged );

    layerThicknessLabel = new QLabel( "Layer thickness:" );
    layerThicknessLabel->setBuddy( layerThicknessComboBox );

    exposureTime = new QLineEdit;
    exposureTime->setAlignment( Qt::AlignRight );
    exposureTime->setText( "1.0" );
    exposureTime->setValidator( new QDoubleValidator( 0.0, 1.0E10, 10 ) );
    QObject::connect( exposureTime, &QLineEdit::editingFinished, this, &Window::exposureTime_editingFinished );

    exposureTimeLabel = new QLabel( "Exposure time (seconds):" );
    exposureTimeLabel->setBuddy( exposureTime );

    exposureScaleFactorComboBox = new QComboBox;
    exposureScaleFactorComboBox->setEditable( false );
    exposureScaleFactorComboBox->setMaxVisibleItems( ExposureScaleFactorStringList.count( ) );
    exposureScaleFactorComboBox->addItems( ExposureScaleFactorStringList );
    printJob->exposureTimeScaleFactor = 1;
    QObject::connect( exposureScaleFactorComboBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &Window::exposureScaleFactorComboBox_currentIndexChanged );

    exposureScaleFactorLabel = new QLabel( "First layers time scale factor:" );
    exposureScaleFactorLabel->setBuddy( exposureScaleFactorComboBox );

    powerLevelSlider = new QSlider( Qt::Orientation::Horizontal );
    powerLevelSlider->setTickPosition( QSlider::TickPosition::TicksBelow );
    powerLevelSlider->setMinimum( 20 );
    powerLevelSlider->setMaximum( 100 );
    powerLevelSlider->setValue( 50 );
    printJob->powerLevel = 127;
    QObject::connect( powerLevelSlider, &QSlider::valueChanged, this, &Window::powerLevelSlider_valueChanged );

    powerLevelLabel = new QLabel( "Projector power level:" );
    powerLevelLabel->setBuddy( powerLevelSlider );

    powerLevelValue = new QLabel( "50%" );
    powerLevelValue->setAlignment( Qt::AlignRight );
    powerLevelValue->setFrameShadow( QFrame::Sunken );
    powerLevelValue->setFrameStyle( QFrame::StyledPanel );

    powerLevelValueLayout = new QHBoxLayout( );
    powerLevelValueLayout->setContentsMargins( emptyMargins );
    powerLevelValueLayout->addWidget( powerLevelLabel );
    powerLevelValueLayout->addStretch( );
    powerLevelValueLayout->addWidget( powerLevelValue );

    powerLevelValueContainer = new QWidget( );
    powerLevelValueContainer->setContentsMargins( emptyMargins );
    powerLevelValueContainer->setLayout( powerLevelValueLayout );

    powerLevelSliderLeftLabel = new QLabel( "20%" );
    powerLevelSliderLeftLabel->setAlignment( Qt::AlignLeft );
    powerLevelSliderRightLabel = new QLabel( "100%" );
    powerLevelSliderRightLabel->setAlignment( Qt::AlignRight );

    powerLevelSliderLabelsLayout = new QHBoxLayout( );
    powerLevelSliderLabelsLayout->addWidget( powerLevelSliderLeftLabel );
    powerLevelSliderLabelsLayout->addStretch( );
    powerLevelSliderLabelsLayout->addWidget( powerLevelSliderRightLabel );

    powerLevelSliderLabelsContainer = new QWidget( );
    powerLevelSliderLabelsContainer->setLayout( powerLevelSliderLabelsLayout );

    optionsLayout = new QVBoxLayout;
    optionsLayout->setContentsMargins( emptyMargins );
    optionsLayout->addWidget( layerThicknessLabel );
    optionsLayout->addWidget( layerThicknessComboBox );
    optionsLayout->addWidget( exposureTimeLabel );
    optionsLayout->addWidget( exposureTime );
    optionsLayout->addWidget( exposureScaleFactorLabel );
    optionsLayout->addWidget( exposureScaleFactorComboBox );
    optionsLayout->addWidget( powerLevelValueContainer );
    optionsLayout->addWidget( powerLevelSlider );
    optionsLayout->addWidget( powerLevelSliderLabelsContainer );
    optionsLayout->addStretch( );

    optionsContainer = new QWidget( );
    optionsContainer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    optionsContainer->setLayout( optionsLayout );

    sliceButton = new QPushButton( "Slice" );
    {
        auto font { sliceButton->font( ) };
        font.setPointSizeF( 22.25 );
        sliceButton->setFont( font );
    }
    sliceButton->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::MinimumExpanding );
    sliceButton->setEnabled( false );
    QObject::connect( sliceButton, &QPushButton::clicked, this, &Window::sliceButton_clicked );

    printButton = new QPushButton( "Print" );
    {
        auto font { printButton->font( ) };
        font.setPointSizeF( 22.25 );
        printButton->setFont( font );
    }
    printButton->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::MinimumExpanding );
    printButton->setEnabled( false );
    QObject::connect( printButton, &QPushButton::clicked, this, &Window::printButton_clicked );

    currentSliceLabel = new QLabel( "Current slice:" );
    currentSliceDisplay = new QLabel;
    currentSliceLabel->setBuddy( currentSliceDisplay );
    currentSliceDisplay->setAlignment( Qt::AlignCenter );
    {
        auto pal = currentSliceDisplay->palette( );
        pal.setColor( QPalette::Background, Qt::black );
        currentSliceDisplay->setPalette( pal );
    }

    currentSliceLayout = new QVBoxLayout;
    currentSliceLayout->setContentsMargins( emptyMargins );
    currentSliceLayout->addWidget( sliceProgressContainer );
    currentSliceLayout->addWidget( currentSliceLabel );
    currentSliceLayout->addWidget( currentSliceDisplay );
    currentSliceLayout->addStretch( );

    currentSliceContainer = new QWidget;
    currentSliceContainer->setContentsMargins( emptyMargins );
    currentSliceContainer->setLayout( currentSliceLayout );
    currentSliceContainer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    currentSliceContainer->setMinimumSize( 600, 400 );

    printTabLayout = new QGridLayout;
    printTabLayout->setContentsMargins( emptyMargins );
    printTabLayout->addWidget( optionsContainer,      0, 0, 1, 1 );
    printTabLayout->addWidget( sliceButton,           1, 0, 1, 1 );
    printTabLayout->addWidget( printButton,           2, 0, 1, 1 );
    printTabLayout->addWidget( currentSliceContainer, 0, 1, 3, 1 );
    printTabLayout->setRowStretch( 0, 3 );
    printTabLayout->setRowStretch( 1, 1 );
    printTabLayout->setRowStretch( 2, 1 );

    printTab = new QWidget;
    printTab->setContentsMargins( emptyMargins );
    printTab->setLayout( printTabLayout );
    printTab->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    //
    // "Status" tab
    //

    printerStateLabel   = new QLabel( "Printer status:" );
    printerStateDisplay = new QLabel( "Offline" );
    printerStateLabel->setBuddy( printerStateDisplay );
    printerStateDisplay->setFrameShadow( QFrame::Sunken );
    printerStateDisplay->setFrameStyle( QFrame::StyledPanel );

    projectorLampStateLabel   = new QLabel( "Projector lamp status:" );
    projectorLampStateDisplay = new QLabel( "Off" );
    projectorLampStateLabel->setBuddy( projectorLampStateDisplay );
    projectorLampStateDisplay->setFrameShadow( QFrame::Sunken );
    projectorLampStateDisplay->setFrameStyle( QFrame::StyledPanel );

    jobStateLabel   = new QLabel( "Job status:" );
    jobStateDisplay = new QLabel( "Not printing" );
    jobStateLabel->setBuddy( jobStateDisplay );
    jobStateDisplay->setFrameShadow( QFrame::Sunken );
    jobStateDisplay->setFrameStyle( QFrame::StyledPanel );

    currentLayerLabel   = new QLabel( "Printer status:" );
    currentLayerDisplay = new QLabel( "Offline" );
    currentLayerLabel->setBuddy( currentLayerDisplay );
    currentLayerDisplay->setFrameShadow( QFrame::Sunken );
    currentLayerDisplay->setFrameStyle( QFrame::StyledPanel );

    progressControlsLayout = new QVBoxLayout;
    progressControlsLayout->setContentsMargins( emptyMargins );
    progressControlsLayout->addWidget( printerStateLabel );
    progressControlsLayout->addWidget( printerStateDisplay );
    progressControlsLayout->addWidget( projectorLampStateLabel );
    progressControlsLayout->addWidget( projectorLampStateDisplay );
    progressControlsLayout->addWidget( jobStateLabel );
    progressControlsLayout->addWidget( jobStateDisplay );
    progressControlsLayout->addWidget( currentLayerLabel );
    progressControlsLayout->addWidget( currentLayerDisplay );
    progressControlsLayout->addStretch( );

    progressControlsContainer = new QWidget;
    progressControlsContainer->setContentsMargins( emptyMargins );
    progressControlsContainer->setLayout( progressControlsLayout );
    progressControlsContainer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    currentLayerImageLabel = new QLabel( "Current layer:" );
    currentLayerImageDisplay = new QLabel;
    currentLayerImageLabel->setBuddy( currentLayerImageDisplay );
    currentLayerImageDisplay->setAlignment( Qt::AlignCenter );
    currentLayerImageDisplay->setMaximumSize( 600, 400 );
    {
        auto pal = currentLayerImageDisplay->palette( );
        pal.setColor( QPalette::Background, Qt::black );
        currentLayerImageDisplay->setPalette( pal );
    }

    currentLayerImageLayout = new QVBoxLayout;
    currentLayerImageLayout->setContentsMargins( emptyMargins );
    currentLayerImageLayout->addWidget( currentLayerImageLabel );
    currentLayerImageLayout->addWidget( currentLayerImageDisplay );
    currentLayerImageLayout->addStretch( );

    currentLayerImageContainer = new QWidget;
    currentLayerImageContainer->setContentsMargins( emptyMargins );
    currentLayerImageContainer->setLayout( currentLayerImageLayout );
    currentLayerImageContainer->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    currentLayerImageContainer->setMinimumSize( 600, 400 );

    stopButton = new QPushButton( "STOP" );
    {
        auto font { stopButton->font( ) };
        font.setPointSizeF( 22.25 );
        stopButton->setFont( font );
    }
    stopButton->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::MinimumExpanding );
    stopButton->setEnabled( false );
    QObject::connect( stopButton, &QPushButton::clicked, this, &Window::stopButton_clicked );

    statusTabLayout = new QGridLayout;
    statusTabLayout->setContentsMargins( emptyMargins );
    statusTabLayout->addWidget( progressControlsContainer,  0, 0, 1, 1 );
    statusTabLayout->addWidget( stopButton,                 1, 0, 1, 1 );
    statusTabLayout->addWidget( currentLayerImageContainer, 0, 1, 2, 1 );
    statusTabLayout->setRowStretch( 0, 4 );
    statusTabLayout->setRowStretch( 1, 1 );

    statusTab = new QWidget;
    statusTab->setContentsMargins( emptyMargins );
    statusTab->setLayout( statusTabLayout );
    statusTab->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    //
    // Tab widget
    //

    tabs = new QTabWidget;
    tabs->setContentsMargins( emptyMargins );
    tabs->addTab( selectTab, "Select" );
    tabs->addTab( printTab,  "Print"  );
    tabs->addTab( statusTab, "Status" );
    tabs->setCurrentIndex( TabIndex::Select );

    setCentralWidget( tabs );

    shepherd = new Shepherd( parent );
    QObject::connect( shepherd, &Shepherd::shepherd_Started,              this, &Window::shepherd_Started              );
    QObject::connect( shepherd, &Shepherd::shepherd_Finished,             this, &Window::shepherd_Finished             );
    QObject::connect( shepherd, &Shepherd::shepherd_ProcessError,         this, &Window::shepherd_ProcessError         );
    QObject::connect( shepherd, &Shepherd::printer_Online,                this, &Window::printer_Online                );
    QObject::connect( shepherd, &Shepherd::printer_Offline,               this, &Window::printer_Offline               );
    shepherd->start( );
}

Window::~Window( ) {
    if ( g_signalHandler ) {
        QObject::disconnect( g_signalHandler, &SignalHandler::quit, this, &Window::signalHandler_quit );
    }
}

void Window::closeEvent( QCloseEvent* event ) {
    fprintf( stderr, "+ Window::closeEvent\n" );
    if ( printManager ) {
        printManager->terminate( );
    }
    shepherd->doTerminate( );
    event->accept( );
}

void Window::shepherd_Started( ) {
    fprintf( stderr, "+ Window::shepherd_Started\n" );
}

void Window::shepherd_Finished( int exitCode, QProcess::ExitStatus exitStatus ) {
    fprintf( stderr, "+ Window::shepherd_Finished: exitStatus %d, exitCode %d\n", exitStatus, exitCode );
}

void Window::shepherd_ProcessError( QProcess::ProcessError error ) {
    fprintf( stderr, "+ Window::shepherd_ProcessError: %d\n", error );
}

void Window::printer_Online( ) {
    fprintf( stderr, "+ Window::printer_Online\n" );
    isPrinterOnline = true;
    printerStateDisplay->setText( "Online" );
}

void Window::printer_Offline( ) {
    fprintf( stderr, "+ Window::printer_Offline\n" );
    isPrinterOnline = false;
    printerStateDisplay->setText( "Offline" );
}

void Window::selectTab_modelLoadComplete( bool const success, QString const& fileName ) {
    fprintf( stderr, "+ Window::selectTab_modelLoadComplete: success: %s, fileName: '%s'\n", success ? "true" : "false", fileName.toUtf8( ).data( ) );
    sliceButton->setEnabled( success );
    printButton->setEnabled( !success );
    if ( success ) {
        tabs->setCurrentIndex( TabIndex::Print );
    }
}

void Window::layerThicknessComboBox_currentIndexChanged( int index ) {
    fprintf( stderr, "+ Window::layerThicknessComboBox_currentIndexChanged: new value: %d µm\n", LayerThicknessValues[index] );
    printJob->layerThickness = LayerThicknessValues[index];
}

void Window::sliceButton_clicked( bool /*checked*/ ) {
    fprintf( stderr, "+ Window::sliceButton_clicked\n" );
    sliceButton->setEnabled( false );
    printButton->setEnabled( false );

    printJob->pngFilesPath = StlModelLibraryPath + QString( "/working_%1" ).arg( static_cast<unsigned long long>( getpid( ) ) * 10000000000ull + static_cast<unsigned long long>( rand( ) ) );
    mkdir( printJob->pngFilesPath.toUtf8( ).data( ), 0700 );
    QString baseName = printJob->modelFileName;
    int index = baseName.lastIndexOf( "/" );
    if ( index > -1 ) {
        baseName = baseName.mid( index + 1 );
    }
    if ( baseName.endsWith( ".stl", Qt::CaseInsensitive ) ) {
        printJob->slicedSvgFileName = printJob->pngFilesPath + QChar( '/' ) + baseName.left( baseName.length( ) - 4 ) + QString( ".svg" );
    } else {
        printJob->slicedSvgFileName = printJob->pngFilesPath + QChar( '/' ) + baseName                                + QString( ".svg" );
    }
    fprintf( stderr,
        "  + model filename:      '%s'\n"
        "  + sliced SVG filename: '%s'\n"
        "  + PNG files path:      '%s'\n"
        "",
        printJob->modelFileName.toUtf8( ).data( ),
        printJob->slicedSvgFileName.toUtf8( ).data( ),
        printJob->pngFilesPath.toUtf8( ).data( )
    );

    slicerProcess = new QProcess( this );
    slicerProcess->setProgram( SlicerCommand );
    slicerProcess->setArguments( QStringList {
        printJob->modelFileName,
        "--export-svg",
        "--layer-height",
        QString( "%1" ).arg( printJob->layerThickness / 1000.0 ),
        "--output",
        printJob->slicedSvgFileName
    } );
    fprintf( stderr, "  + command line:        '%s %s'\n", slicerProcess->program( ).toUtf8( ).data( ), slicerProcess->arguments( ).join( QChar( ' ' ) ).toUtf8( ).data( ) );
    QObject::connect( slicerProcess, &QProcess::errorOccurred, this, &Window::slicerProcessErrorOccurred );
    QObject::connect( slicerProcess, &QProcess::started,       this, &Window::slicerProcessStarted       );
    QObject::connect( slicerProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this, &Window::slicerProcessFinished );
    slicerProcess->start( );
}

void Window::exposureTime_editingFinished( ) {
    bool valueOk = false;
    double value = exposureTime->validator( )->locale( ).toDouble( exposureTime->text( ), &valueOk );
    if ( valueOk ) {
        fprintf( stderr, "+ Window::exposureTime_editingFinished: new value %f\n", value );
        printJob->exposureTime = value;
    } else {
        fprintf( stderr, "+ Window::exposureTime_editingFinished: bad value\n" );
    }
}

void Window::exposureScaleFactorComboBox_currentIndexChanged( int index ) {
    fprintf( stderr, "+ Window::exposureScaleFactorComboBox_currentIndexChanged: new value: %d×\n", ExposureScaleFactorValues[index] );
    printJob->exposureTimeScaleFactor = ExposureScaleFactorValues[index];
}

void Window::powerLevelSlider_valueChanged( int value ) {
    int scaledValue = ( value / 100.0 * 255.0 ) + 0.5;
    printJob->powerLevel = scaledValue;
    powerLevelValue->setText( QString( "%1%" ).arg( value ) );
}

void Window::printButton_clicked( bool /*checked*/ ) {
    fprintf( stderr, "+ Window::printButton_clicked\n" );
    tabs->setCurrentIndex( TabIndex::Status );

    fprintf( stderr,
        "  + Print job:\n"
        "    + modelFileName:     '%s'\n"
        "    + slicedSvgFileName: '%s'\n"
        "    + pngFilesPath:      '%s'\n"
        "    + layerCount:        %d\n"
        "    + layerThickness:    %d\n"
        "    + exposureTime:      %f\n"
        "    + powerLevel:        %d\n"
        "",
        printJob->modelFileName.toUtf8( ).data( ),
        printJob->slicedSvgFileName.toUtf8( ).data( ),
        printJob->pngFilesPath.toUtf8( ).data( ),
        printJob->layerCount,
        printJob->layerThickness,
        printJob->exposureTime,
        printJob->powerLevel
    );

    PrintJob* newJob = new PrintJob;
    *newJob = *printJob;

    printManager = new PrintManager( shepherd, this );
    printManager->print( printJob );

    printJob = newJob;

    stopButton->setEnabled( true );
}

void Window::stopButton_clicked( bool /*checked*/ ) {
    printManager->abortJob( );
}

void Window::printManager_printStarting( ) {
    jobStateDisplay->setText( "Print started" );
}

void Window::printManager_printingLayer( int const layer ) {
    currentLayerDisplay->setText( QString( "%1" ).arg( layer ) );
    currentLayerImageDisplay->setPixmap( QPixmap( QString( "%1/%2.png" ).arg( printJob->pngFilesPath ).arg( layer, 6, 10, QChar( '0' ) ) ) );
}

void Window::printManager_lampStatusChange( bool const on ) {
    projectorLampStateDisplay->setText( on ? QString( "On" ) : QString( "Off" ) );
}

void Window::printManager_printComplete( bool const success ) {
    jobStateDisplay->setText( success ? "Print complete" : "Print failed" );
}

void Window::slicerProcessErrorOccurred( QProcess::ProcessError error ) {
    fprintf( stderr, "+ Window::slicerProcessErrorOccurred: error %s [%d]\n", ToString( error ), error );

    if ( QProcess::FailedToStart == error ) {
        fprintf( stderr, "  + slicer process failed to start\n" );
        sliceProgress->setText( "Slicer failed to start" );
    } else if ( QProcess::Crashed == error ) {
        fprintf( stderr, "  + slicer process crashed? state is %s [%d]\n", ToString( slicerProcess->state( ) ), slicerProcess->state( ) );
        if ( slicerProcess->state( ) != QProcess::NotRunning ) {
            slicerProcess->kill( );
            fprintf( stderr, "  + slicer terminated\n" );
        }
        sliceProgress->setText( "Slicer crashed" );
    }
}

void Window::slicerProcessStarted( ) {
    fprintf( stderr, "+ Window::slicerProcessStarted\n" );
    sliceProgress->setText( "Slicer started" );
}

void Window::slicerProcessFinished( int exitCode, QProcess::ExitStatus exitStatus ) {
    QObject::disconnect( slicerProcess, &QProcess::errorOccurred, this, &Window::slicerProcessErrorOccurred );
    QObject::disconnect( slicerProcess, &QProcess::started,       this, &Window::slicerProcessStarted       );
    QObject::disconnect( slicerProcess, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this, &Window::slicerProcessFinished );

    fprintf( stderr, "+ Window::slicerProcessFinished: exitCode: %d, exitStatus: %s [%d]\n", exitCode, ToString( exitStatus ), exitStatus );

    delete slicerProcess;
    slicerProcess = nullptr;

    if ( exitStatus == QProcess::CrashExit ) {
        fprintf( stderr, "  + slicer process crashed?\n" );
        sliceProgress->setText( "Slicer crashed" );
        return;
    }

    sliceProgress->setText( "Slicer finished" );

    svgRenderer = new SvgRenderer;
    QObject::connect( svgRenderer, &SvgRenderer::nextLayer, this, &Window::svgRenderer_progress );
    QObject::connect( svgRenderer, &SvgRenderer::done,      this, &Window::svgRenderer_done     );
    svgRenderer->startRender( printJob->slicedSvgFileName, printJob->pngFilesPath );
}

void Window::svgRenderer_progress( int const currentLayer ) {
    if ( 0 == ( currentLayer % 5 ) ) {
        renderProgress->setText( QString( "Rendering layer %1" ).arg( currentLayer ) );
        if ( currentLayer > 0 ) {
            auto pngFileName = QString( "%1/%2.png" ).arg( printJob->pngFilesPath ).arg( currentLayer - 1, 6, 10, QChar( '0' ) );
            auto pixMap = QPixmap( pngFileName );
            currentSliceDisplay->setPixmap( pixMap );
        }
    }
}

void Window::svgRenderer_done( int const totalLayers ) {
    if ( totalLayers == -1 ) {
        renderProgress->setText( QString( "Rendering failed" ) );
    } else {
        renderProgress->setText( QString( "Rendering complete" ) );
        printJob->layerCount = totalLayers;
        printButton->setEnabled( true );
    }

    QObject::disconnect( svgRenderer, &SvgRenderer::nextLayer, this, &Window::svgRenderer_progress );
    QObject::disconnect( svgRenderer, &SvgRenderer::done,      this, &Window::svgRenderer_done     );
    delete svgRenderer;
    svgRenderer = nullptr;
}

void Window::signalHandler_quit( int signalNumber ) {
    fprintf( stderr, "+ Window::signalHandler_quit: received signal %d\n", signalNumber );
    close( );
}
