
#include "glframe_retrace_images.hpp"
#include <assert.h>

using glretrace::FrameImages;

FrameImages * FrameImages::m_instance = NULL;

FrameImages *FrameImages::instance()
{
    return m_instance;
}
void FrameImages::Create()
{
    assert(m_instance == NULL);
    m_instance = new FrameImages();
}
void
FrameImages::Destroy() {
    assert(m_instance != NULL);
    delete m_instance;
    m_instance = NULL;
}


