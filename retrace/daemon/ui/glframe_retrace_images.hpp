#ifndef GLFRAME_RETRACE_IMAGES_HPP__
#define GLFRAME_RETRACE_IMAGES_HPP__


#include <QObject>
#include <QImage>
#include <QQuickImageProvider>
#include <vector>

namespace glretrace
{
class FrameImages : public QQuickImageProvider
{
public:
    static FrameImages *instance();
    static void Create();
    static void Destroy();
    virtual QImage requestImage(const QString & id,
                                QSize * size,
                                const QSize & requestedSize)
    {
        return m_rt;
    }
    void SetImage(const std::vector<unsigned char> &buf)
    {
        m_rt.loadFromData(buf.data(), buf.size(), "PNG");
    }
private:
    FrameImages() : QQuickImageProvider(QQmlImageProviderBase::Image) {}
    QImage m_rt;
    static FrameImages *m_instance;
};

}

#endif
