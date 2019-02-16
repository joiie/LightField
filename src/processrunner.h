#ifndef __PROCESSRUNNER_H__
#define __PROCESSRUNNER_H__

class ProcessRunner: public QObject {

    Q_OBJECT

public:

    ProcessRunner( QObject* parent = nullptr );
    virtual ~ProcessRunner( ) override;

    void start( QString const& program, QStringList const& arguments, QProcess::OpenMode const mode = QProcess::ReadWrite );

    QProcess::ProcessState state( ) const {
        return _process.state( );
    }

protected:

private:

    QProcess _process;

signals:

    void succeeded( );
    void failed( QProcess::ProcessError const error );

public slots:

    void kill( ) {
        _process.kill( );
    }

protected slots:

private slots:

    void processErrorOccurred( QProcess::ProcessError error );
    void processFinished( int exitCode, QProcess::ExitStatus exitStatus );

};

#endif // __PROCESSRUNNER_H__