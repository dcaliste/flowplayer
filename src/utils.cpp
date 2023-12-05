#include "utils.h"
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QNetworkConfigurationManager>
#include <QSettings>
//#include <MGConfItem>

QString albumArtUrl, albumArtArtist, albumArtAlbum;
QString currentArtist, currentSong;
QString searchServer;

QSettings settings("cepiperez", "flowplayer");

//extern bool isDBOpened;

Utils::Utils(QQuickItem *parent)
    : QQuickItem(parent)
{
    datos = new QNetworkAccessManager(this);
    connect(datos, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloaded(QNetworkReply*)));
    currentLyrics = "";
}

bool Utils::isOnline()
{
    QNetworkConfigurationManager mgr;
    QList<QNetworkConfiguration> activeConfigs = mgr.allConfigurations(QNetworkConfiguration::Active);
    if (activeConfigs.count() > 0)
        return true;
    else
        return false;
}

bool Utils::isFav(QString filename)
{
    return executeQueryCheckCount(QString("select * from tracks where url='%1' and fav=1").arg(filename));
}

void Utils::favSong(QString filename, bool fav)
{
    QString val = fav? "1" : "NULL";
    executeQuery(QString("update tracks set fav=%1 where url='%2'").arg(val).arg(filename));
}

void Utils::startMafw(QString artist, QString title)
{

    /*QDBusInterface iface("com.nokia.maemo.meegotouch.MusicSuiteInterface", "/",
                         "com.nokia.maemo.meegotouch.MusicSuiteInterface",
                         QDBusConnection::systemBus(), this);

    qDebug() << "SENDING DBUS SIGNAL " << title << artist;
    iface.call("mediaChanged", "0", title, artist);

    QString RENDERER_UUID = "mafw_gst_renderer";
    qDebug() << "HELLO WORLD";
    //MafwMessageHandler::initMafwLogging(true);
    MafwRegistry* registry = MafwRegistry::instance();
    MafwShared* shared = MafwShared::instance();
    bool initialized = shared->initTracking(registry);
    qDebug() << "MafwShared initialization result was: " << initialized;

    QList<MafwSource*> sources = MafwRegistry::instance()->sources();
    qDebug() << sources.count() << " sources found.";
    //Then list names and uuids for each source.
    qDebug() << "Name - Uuid";

    registry = MafwRegistry::instance();
    MafwRenderer* renderer = registry->renderer(RENDERER_UUID);
    if (renderer)
    {
        renderer->play(QUrl("file:///home/nemo/MyDocs/Music/Alexisonfire/Alexisonfire/Adelleda.mp3"));
    }*/



}

void Utils::readLyrics(QString artist, QString song)
{
    QString art = cleanItem(artist);
    QString sng = cleanItem(song);
    if ( ( art!="" ) && ( sng!="" ) )
    {
        QString th1 = "/home/nemo/.cache/lyrics/"+art+"-"+sng+".txt";

        if ( QFileInfo(th1).exists() )
        {
            QString lines;
            QFile data(th1);
            if (data.open(QFile::ReadOnly | QFile::Truncate))
            {
                QTextStream out(&data);
                while ( !out.atEnd() )
                    lines += out.readLine()+"\n";
            }
            data.close();
            currentLyrics = lines.replace("\n", "<br>");
            m_noLyrics = false;
        }
        else
        {
            currentLyrics =  tr("No lyrics founded");
            m_noLyrics = true;
        }
    }
    m_lyricsonline = false;
    //qDebug() << art << sng << currentLyrics.left(20);
    emit lyricsChanged();

}

QString Utils::thumbnail(QString artist, QString album, QString count)
{
    QString art = count=="1"? artist : album;
    QString alb = album;

    QString th1 = "/home/nemo/.cache/media-art/album-"+ doubleHash(art, alb) + ".jpeg";

    if (!QFileInfo(th1).exists()) {
        QString th2 = "/home/nemo/.cache/media-art/album-"+ doubleHash(alb, alb) + ".jpeg";
        if (QFileInfo(th2).exists())
            return th2;
    }

    return th1;
}

QString Utils::thumbnailArtist(QString artist)
{
    QString th1 = "/home/nemo/.cache/flowplayer/artist-"+ hash(artist) + ".jpeg";
    return th1;
}

QString Utils::accents(QString data)
{
    //qDebug() << data;
    QString str = QString::fromUtf8(data.toUtf8());
    str.replace("á", "a");
    str.replace("é", "e");
    str.replace("í", "i");
    str.replace("ó", "o");
    str.replace("ú", "u");
    return str;
}


void Utils::getLyrics(QString artist, QString song, QString server)
{
    searchServer = server;

    //if (reply->isRunning())
    //    reply->abort();


    //qDebug() << "seaching in " << server;
    /*QNetworkConfigurationManager mgr;
    QList<QNetworkConfiguration> activeConfigs = mgr.allConfigurations(QNetworkConfiguration::Active);
    if ( activeConfigs.count() > 0 )
    {*/
        if ( searchServer == "0" )
        {
            qDebug() << "Searching in ChartLyrics";
            QString url = "http://api.chartlyrics.com/apiv1.asmx/SearchLyricDirect";
            reply = datos->get(QNetworkRequest(QUrl(url+"?Artist=\""+artist.toLower()+"\"&Song=\""+song.toLower()+"\"")));
        }
        else if ( searchServer == "1" )
        {
            QString url = "http://www.azlyrics.com/lyrics/"+ cleanItem(artist) + "/" + cleanItem(song) +".html";
            qDebug() << "Searching in A-Z Lyrics " << url;
            reply = datos->get(QNetworkRequest(QUrl(url)));
        }
        else if ( searchServer == "2" )
        {
            QString url = "http://lyrics.wikia.com/wiki/"+ artist.replace(" ", "_") + ":" + song.replace(" ", "_");
            qDebug() << "Searching in Lyric Wiki " << url;
            reply = datos->get(QNetworkRequest(QUrl(url.replace(" ","_"))));
        }
        else if ( searchServer == "3" )
        {
            //qDebug() << "Searching in LyricsDB";
            QString url = "http://lyrics.mirkforce.net/"+ cleanItem(artist) + "/" + cleanItem(song) +".txt";
            reply = datos->get(QNetworkRequest(QUrl(url)));
        }
    /*}
    else
    {
        QString line1 = tr("There is not an active Internet connection");
        QString line2 = tr("Please connect to use this function.");
        currentLyrics = line1+"\n"+line2;
        emit lyricsChanged();
    }*/

}

void Utils::downloaded(QNetworkReply *respuesta)
{
    QString datos1;

    if (respuesta->error() != QNetworkReply::NoError)
    {
        qDebug() << "error: " << respuesta->error();
        {
            if ( searchServer == "albumart ")
                albumArtUrl = "";
            else
            {
                currentLyrics = tr("Error fetching lyrics");
                m_lyricsonline = true;
                m_noLyrics = true;
                emit lyricsChanged();
            }
        }
    }
    else
    {
        if ( searchServer == "autorization" )
        {
            datos1 = respuesta->readAll();
            //qDebug() << "SIGNATURE: " << datos1;
        }
        else if ( searchServer == "albumart" )
        {
            datos1 = respuesta->readAll();
            //qDebug() << datos1;
            QString tmp = datos1;
            int x = tmp.indexOf("<image size=\"mega\">");
            tmp.remove(0,x+19);
            x = tmp.indexOf("<");
            tmp.remove(x,tmp.length()-x);
            tmp = tmp.trimmed();
            //qDebug() << "Image founded... downloading... " << tmp;
            if (tmp=="") {
                banner = tr("Album cover not founded");
                emit bannerChanged();
                return;
            }
            /*QUrl url(tmp);
            http = new QHttp(this);
            connect(http, SIGNAL(requestFinished(int, bool)),this, SLOT(Finished(int,
            bool)));
            buffer = new QBuffer(&bytes);
            buffer->open(QIODevice::WriteOnly);
            http->setHost(url.host());
            Request=http->get (url.path(),buffer);*/

        }

        else if ( searchServer == "3" )
        {
            QString str = QString::fromUtf8(respuesta->readAll());
            qDebug() << str;
            if ( str.contains("was not found on this server") )
            {
                currentLyrics = tr("No lyrics founded");
                m_noLyrics = true;
            }
            else
            {
                currentLyrics = str.replace("\n", "<br>");
                m_noLyrics = false;
            }
            m_lyricsonline = true;
            emit lyricsChanged();
        }
        else if ( searchServer == "2" )
        {
            datos1 = QString::fromUtf8(respuesta->readAll());
            qDebug() << datos1;
            if ( datos1.contains("ntNode.insertBefore(r,s)};}})();</script>") )
            {
                QString tmp = datos1;
                int x = tmp.indexOf("ntNode.insertBefore(r,s)};}})();</script>");
                tmp.remove(0,x+41);
                x = tmp.indexOf("<!--");
                tmp.remove(x,tmp.length()-x);
                tmp.replace("<br />", "<br>");
                tmp.replace("\n\n", "\n");
                currentLyrics = tmp.replace("\n", "<br>");

                if (currentLyrics.contains("<div ")) {
                    currentLyrics = tr("No lyrics founded");
                    m_noLyrics = true;
                } else {
                    m_noLyrics = false;
                }
            }
            else
            {
                currentLyrics = tr("No lyrics founded");
                m_noLyrics = true;
            }
            m_lyricsonline = true;
            emit lyricsChanged();
        }
        else if ( searchServer == "1" )
        {
            datos1 = QString::fromUtf8(respuesta->readAll());
            qDebug() << datos1;
            if ( datos1.contains("<!-- start of lyrics -->") )
            {
                QString tmp = datos1;
                int x = tmp.indexOf("<!-- start of lyrics -->");
                tmp.remove(0,x+24);
                x = tmp.indexOf("<!-- end of lyrics -->");
                tmp.remove(x,tmp.length()-x);
                tmp.replace("<br>", "\n");
                tmp.remove("\r");
                tmp.remove("<i>");
                tmp.remove("</i>");
                tmp.remove(0,1);
                tmp.replace("<br />", "\n");
                tmp.replace("\n\n", "\n");
                currentLyrics = tmp.replace("\n", "<br>");
                m_noLyrics = false;
            }
            else
            {
                currentLyrics = tr("No lyrics founded");
                m_noLyrics = true;
            }
            m_lyricsonline = true;
            emit lyricsChanged();
        }
        else
        {
            datos1 = respuesta->readAll();
            qDebug() << datos1;
            if ( datos1.contains("<Lyric>") )
            {
                QString tmp = datos1;
                int x = tmp.indexOf("<Lyric>");
                tmp.remove(0,x+7);
                x = tmp.indexOf("<");
                tmp.remove(x,tmp.length()-x);
                tmp.replace("\r", "\n");
                tmp.replace("\n\n", "\n");
                currentLyrics = tmp.replace("\n", "<br>");
                m_noLyrics = false;
            }
            else
            {
                currentLyrics = tr("No lyrics founded");
                m_noLyrics = true;
            }
            m_lyricsonline = true;
            emit lyricsChanged();
        }
    }


}

void Utils::saveLyrics(QString artist, QString song, QString lyrics)
{
    QDir d;
    d.mkdir("/home/nemo/.cache/lyrics");

    QString art = cleanItem(artist);
    QString sng = cleanItem(song);
    QString f = "/home/nemo/.cache/lyrics/"+art+"-"+sng+".txt";

    if ( QFileInfo(f).exists() )
        QFile::remove(f);
    QFile file(f);
    file.open( QIODevice::Truncate | QIODevice::Text | QIODevice::ReadWrite);
    QTextStream out(&file);
    //QTextEdit texto(0);
    //texto.setText(lyrics);
    out << lyrics;
    file.close();
    m_lyricsonline = false;
    emit lyricsChanged();
}

void Utils::saveLyrics2(QString artist, QString song, QString lyrics)
{
    QDir d;
    d.mkdir("/home/nemo/.cache/lyrics");

    QString art = cleanItem(artist);
    QString sng = cleanItem(song);
    QString f = "/home/nemo/.cache/lyrics/"+art+"-"+sng+".txt";

    if ( QFileInfo(f).exists() )
        QFile::remove(f);
    QFile file(f);
    file.open( QIODevice::Truncate | QIODevice::Text | QIODevice::ReadWrite);
    QTextStream out(&file);
    //out.setCodec("UTF-8");
    out << lyrics;
    file.close();

}

void Utils::cancelFetching()
{
    if ( reply && reply->isRunning() )
        reply->abort();
    emit fetchCanceled();
}

void Utils::getAlbumArt(QString artist, QString album)
{
    emit downloadingCover();
    searchServer = "albumart";
    albumArtArtist = artist;
    albumArtAlbum = album;
    //qDebug() << "Searching album art..." << albumArtArtist << albumArtAlbum;
    QString url = "http://ws.audioscrobbler.com/2.0/?method=album.getinfo&api_key=7f338c7458e7d1a9a6204221ff904ba1";
    reply = datos->get(QNetworkRequest(QUrl(url+"&artist="+albumArtArtist+"&album="+albumArtAlbum)));
}

void Utils::Finished(int requestId, bool)
{
    if ( searchServer == "albumart" )
    {
        if (Request==requestId)
        {
            QImage img;
            img.loadFromData(bytes);

            QString th1 = "/home/nemo/.cache/media-art/album-" + doubleHash(albumArtArtist, albumArtAlbum) + ".jpeg";
            if ( QFileInfo("/home/nemo/.cache/media-art/preview.jpeg").exists() )
                removePreview();
            img.save("/home/nemo/.cache/media-art/preview.jpeg");
            downloadedAlbumArt = th1;
            emit coverDownloaded();
        }
    }

}

void Utils::removePreview()
{
    QFile f("/home/nemo/.cache/media-art/preview.jpeg");
    f.remove();
}

void Utils::setSettings(QString set, QString val)
{
    settings.setValue(set, val);
    settings.sync();
}

QString Utils::readSettings(QString set, QString val)
{
    return settings.value(set, val).toString();
}

QString Utils::showReflection()
{
    return settings.value("ShowReflection", "true").toString();
}

QString Utils::viewmode() const
{
    return settings.value("ViewMode", "grid").toString();
}

QString Utils::paging() const
{
    return settings.value("Paging", "multiple").toString();
}

QString Utils::scrobble() const
{
    return settings.value("Scrobble", "false").toString();
}

QString Utils::order() const
{
    return settings.value("SortOrder", "album").toString();
}

QString Utils::lang() const
{
    return settings.value("LastFMlang", "en").toString();
}

QString Utils::updatestart() const
{
    return settings.value("UpdateOnStartup", "no").toString();
}

QString Utils::autosearch() const
{
    return settings.value("AutoSearchLyrics", "yes").toString();
}

QString Utils::cleanqueue() const
{
    return settings.value("CleanQueue", "yes").toString();
}

QString Utils::workoffline() const
{
    return settings.value("WorkOffline", "no").toString();
}


void Utils::setViewMode(QString val)
{
    settings.setValue("ViewMode", val);
    settings.sync();
    emit viewmodeChanged();
}

void Utils::setPaging(QString val)
{
    settings.setValue("Paging", val);
    settings.sync();
    emit pagingChanged();
}

void Utils::setScrobble(QString val)
{
    settings.setValue("Scrobble", val);
    settings.sync();
    emit scrobbleChanged();
}

void Utils::setOrder(QString val)
{
    settings.setValue("SortOrder", val);
    settings.sync();
    emit orderChanged();
}

void Utils::setLang(QString val)
{
    settings.setValue("LastFMlang", val);
    settings.sync();
    emit langChanged();
}

void Utils::setUpdateStart(QString val)
{
    settings.setValue("UpdateOnStartup", val);
    settings.sync();
    emit updateChanged();
}

void Utils::setAutoSearch(QString val)
{
    settings.setValue("AutoSearchLyrics", val);
    settings.sync();
    emit autosearchChanged();
}

void Utils::setCleanQueue(QString val)
{
    settings.setValue("CleanQueue", val);
    settings.sync();
    emit queueChanged();
}

void Utils::setWorkOffline(QString val)
{
    settings.setValue("WorkOffline", val);
    settings.sync();
    emit workofflineChanged();
}

QString Utils::orientation() const
{
    return settings.value("Orientation", "auto").toString();
}

void Utils::setOrientation(QString val)
{
    settings.setValue("Orientation", val);
    settings.sync();
    emit orientationChanged();
}

QString Utils::plainLyrics(QString text)
{
    //QTextEdit texto(0);
    //texto.setText(text);
    return text; //.toPlainText();
}

QString Utils::version()
{
    return settings.value("Firmware", "PR10").toString();
}

QString Utils::reemplazar1(QString data)
{
    data.replace("&","&amp;");
    data.replace("<","&lt;");
    data.replace(">","&gt;");
    data.replace("\"","&quot;");
    data.replace("\'","&apos;");
    return data;
}

QString Utils::reemplazar2(QString data)
{
    data.replace("&amp;", "&");
    data.replace("&lt;", "<");
    data.replace("&gt;", ">");
    data.replace("&quot;", "\"");
    data.replace("&apos;", "\'");
    return data;
}

void Utils::createAlbumArt(QString imagepath)
{
    removeAlbumArt();
    QFile::link(imagepath, "/home/nemo/.cache/currentAlbumArt.jpeg");
}

void Utils::removeAlbumArt()
{
    QFile::remove("/home/nemo/.cache/currentAlbumArt.jpeg");
}

void Utils::getFolders()
{
    QStringList folders = settings.value("Folders","").toString().split("<separator>");
    folders.removeAll("");

    if (folders.count()==0)
        return;

    for (int i=0; i<folders.count(); ++i) {
        QString name = QFileInfo(folders.at(i)).fileName();
        QString path = folders.at(i);
        emit addFolder(name, path);
    }
}

void Utils::getFolderItemsUp(QString path)
{
    if (path.endsWith("/"))
        path.chop(1);
    int i = path.lastIndexOf("/");
    path = path.left(i);
    if (path=="") path = "/";
    getFolderItems(path);
}

void Utils::getFolderItems(QString path)
{
    qDebug() << "Loading folder: " << path;

    if (!QFileInfo(path).exists())
        return;

    QDir dir (path);
    QStringList data;
    //data << ".mp3" << "*.m4a" << "*.wma" << "*.flac" << "*.ogg" << "*.wav" << "*.asf";
    //dir.setNameFilters(data);

    QFileInfoList entries;
    entries = dir.entryInfoList(QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot ,
                                QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);


    QListIterator<QFileInfo> entriesIterator (entries);
    while(entriesIterator.hasNext())
    {
        QFileInfo fileInfo = entriesIterator.next();

        if (fileInfo.isDir())
            emit appendFile(fileInfo.fileName(), fileInfo.absoluteFilePath(), "folder");
        else if (fileInfo.fileName().endsWith(".mp3") || fileInfo.fileName().endsWith(".m4a") ||
                 fileInfo.fileName().endsWith(".wma") || fileInfo.fileName().endsWith(".ogg") ||
                 fileInfo.fileName().endsWith(".flac") || fileInfo.fileName().endsWith(".wav") ||
                 fileInfo.fileName().endsWith(".asf"))
            emit appendFile(fileInfo.fileName(), fileInfo.absoluteFilePath(), "sounds");
    }

    emit appendFilesDone(path);
}

void Utils::addFolderToList(QString path)
{
    QStringList folders = settings.value("Folders","").toString().split("<separator>");
    folders.removeAll("");
    folders.append(path);
    folders.removeDuplicates();
    settings.setValue("Folders", folders.join("<separator>"));
    settings.sync();
    emit foldersChanged();
}

void Utils::removeFolder(QString path)
{
    QStringList folders = settings.value("Folders","").toString().split("<separator>");
    folders.removeAll("");
    folders.removeAll(path);
    folders.removeDuplicates();
    settings.setValue("Folders", folders.join("<separator>"));
    settings.sync();
    //emit foldersChanged();
}

void Utils::setShuffle(int nitems)
{
    itemstotal = nitems;

    items.clear();
    QList<int> temp;

    for (int i=0; i<itemstotal; ++i)
        temp.append(i);

    std::srand(time(0));
    std::random_shuffle(temp.begin(), temp.end());

    while (!temp.isEmpty()) {
        //int i = temp.takeAt(temp.count() == 1 ? 0 : (qrand() % (temp.count() - 1)));
        int i = temp.takeFirst(); // use with random_shuffle
        items.append(i);
    }

    qDebug() << "SHUFFLE LIST ORDER::: " << items;
}

int Utils::getShuffleTrack(int current)
{
    items.removeAll(current);

    int res;
    if (items.count()==0)
        setShuffle(itemstotal);

    res = items.takeFirst();

    //qDebug() << "SHUFFLE ::: " << items;
    return res;
}

void Utils::loadPresets()
{
    //if (!isDBOpened) openDatabase();
    QSqlQuery query = getQuery("select * from presets order by name");

    while( query.next() )
        emit appendPreset(query.value(0).toString());

}

void Utils::removePreset(QString name)
{
    bool res = executeQuery(QString("delete from presets where name='%1'").arg(name));
    qDebug() << "Removing preset " << name << res;
}

void Utils::updateSongDuration(QString source, int duration)
{
    //if (!isDBOpened) openDatabase();

    QString d = QString::number(duration);

    QString qr = QString("UPDATE tracks SET duration='%1' where url='%2'").arg(d).arg(source);
    executeQuery(qr);

    qr = QString("UPDATE playlists SET duration='%1' where url='%2'").arg(d).arg(source);
    executeQuery(qr);

    qr = QString("UPDATE queue SET duration='%1' where url='%2'").arg(d).arg(source);
    executeQuery(qr);

}
