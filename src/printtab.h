#ifndef __PRINTTAB_H__
#define __PRINTTAB_H__

enum class BuildPlatformState {
    Lowered,
    Raising,
    Raised,
    Lowering,
};

class PrintJob;
class Shepherd;

class PrintTab: public QWidget {

    Q_OBJECT

public:

    PrintTab( QWidget* parent = nullptr );
    virtual ~PrintTab( ) override;

    bool      isPrintButtonEnabled( ) const { return printButton->isEnabled( ); }
    PrintJob* printJob( )             const { return _printJob;                 }
    Shepherd* shepherd( )             const { return _shepherd;                 }

protected:

    void showEvent( QShowEvent* event );

private:

    BuildPlatformState _buildPlatformState             { BuildPlatformState::Lowered };
    PrintJob*          _printJob                       { };
    Shepherd*          _shepherd                       { };

    QLabel*            exposureTimeLabel               { new QLabel      };
    QLabel*            exposureTimeValue               { new QLabel      };
    QHBoxLayout*       exposureTimeValueLayout         {                 };
    QWidget*           exposureTimeValueContainer      { new QWidget     };
    QDial*             exposureTimeDial                { new QDial       };
    QLabel*            exposureTimeDialLeftLabel       { new QLabel      };
    QLabel*            exposureTimeDialRightLabel      { new QLabel      };
    QHBoxLayout*       exposureTimeDialLabelsLayout    {                 };
    QWidget*           exposureTimeDialLabelsContainer { new QWidget     };
    QLabel*            exposureTimeScaleFactorLabel    { new QLabel      };
    QComboBox*         exposureTimeScaleFactorComboBox { new QComboBox   };
    QLabel*            powerLevelLabel                 { new QLabel      };
    QLabel*            powerLevelValue                 { new QLabel      };
    QHBoxLayout*       powerLevelValueLayout           {                 };
    QWidget*           powerLevelValueContainer        { new QWidget     };
    QDial*             powerLevelDial                  { new QDial       };
    QLabel*            powerLevelDialLeftLabel         { new QLabel      };
    QLabel*            powerLevelDialRightLabel        { new QLabel      };
    QHBoxLayout*       powerLevelDialLabelsLayout      {                 };
    QWidget*           powerLevelDialLabelsContainer   { new QWidget     };
    QVBoxLayout*       optionsLayout                   { new QVBoxLayout };
    QWidget*           optionsContainer                { new QWidget     };
    QPushButton*       printButton                     { new QPushButton };

    QPushButton*       _raiseOrLowerButton             { new QPushButton };
    QPushButton*       _homeButton                     { new QPushButton };

    QVBoxLayout*       _raiseOrLowerLayout             { new QVBoxLayout };
    QVBoxLayout*       _homeLayout                     { new QVBoxLayout };

    QGroupBox*         _raiseOrLowerGroup              { new QGroupBox   };
    QGroupBox*         _homeGroup                      { new QGroupBox   };

    QGridLayout*       _adjustmentsLayout              { new QGridLayout };
    QGroupBox*         _adjustmentsGroup               { new QGroupBox   };

    QGridLayout*       _layout                         { new QGridLayout };

    std::function<void( QShowEvent* )> _initialShowEventFunc;

    void _initialShowEvent( QShowEvent* event );

signals:

    void printButtonClicked( );
    void raiseBuildPlatform( );
    void homePrinter( );
    void lowerBuildPlatform( );

public slots:

    void setAdjustmentButtonsEnabled( bool const value );
    void setPrintButtonEnabled( bool const value );
    void setPrintJob( PrintJob* printJob );
    void setShepherd( Shepherd* shepherd );

    void raiseBuildPlatformComplete( bool const success );
    void homeComplete( bool const success );
    void lowerBuildPlatformComplete( bool const success );

protected slots:

private slots:

    void exposureTimeDial_valueChanged( int value );
    void exposureTimeScaleFactorComboBox_currentIndexChanged( int index );
    void powerLevelDial_valueChanged( int value );
    void printButton_clicked( bool );
    void _raiseOrLowerButton_clicked( bool );
    void _homeButton_clicked( bool );

};

#endif // __PRINTTAB_H__
