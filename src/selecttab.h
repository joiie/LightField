#ifndef __SELECTTAB_H__
#define __SELECTTAB_H__

class Canvas;
class Loader;

class SelectTab: public QWidget {

    Q_OBJECT

public:

    SelectTab( QWidget* parent = nullptr );
    virtual ~SelectTab( ) override;

    QString fileName( ) {
        return _fileName;
    }

protected:

private:

    QFileSystemModel* _fileSystemModel         { new QFileSystemModel };
    QListView*        _availableFilesListView  { new QListView        };
    QLabel*           _availableFilesLabel     { new QLabel           };
    QGridLayout*      _availableFilesLayout    { new QGridLayout      };
    QWidget*          _availableFilesContainer { new QWidget          };
    QPushButton*      _selectButton            { new QPushButton      };
    Canvas*           _canvas                  {                      };
    Loader*           _loader                  {                      };
    QGridLayout*      _layout                  { new QGridLayout      };
    QString           _fileName;

    bool _loadModel( QString const& filename );

signals:

    void modelLoadComplete( bool const success, QString const& fileName );

public slots:

protected slots:

private slots:

    void                 loader_ErrorBadStl      ( );
    void                 loader_ErrorEmptyMesh   ( );
    void                 loader_ErrorMissingFile ( );
    void                 loader_Finished         ( );
    void                 loader_LoadedFile       ( QString const& filename );
    void        fileSystemModel_DirectoryLoaded  ( QString const& name );
    void        fileSystemModel_FileRenamed      ( QString const& path, QString const& oldName, QString const& newName );
    void        fileSystemModel_RootPathChanged  ( QString const& newPath );
    void availableFilesListView_clicked          ( QModelIndex const& index );
    void           selectButton_clicked          ( bool checked );

};

#endif // __SELECTTAB_H__
