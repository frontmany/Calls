#include "frameProcessor.h"
#include "media/h264Encoder.h"
#include "utilities/constant.h"
#include "utilities/logger.h"

#include <QVideoFrame>
#include <QImage>
#include <QPainter>
#include <QBuffer>
#include <QCursor>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstring>
#endif

FrameProcessor::FrameProcessor(QObject* parent)
    : QObject(parent)
    , m_screenEncoder(nullptr)
    , m_cameraEncoder(nullptr)
    , m_isCameraActive(false)
    , m_isScreenActive(false)
{
    LOG_INFO("FrameProcessor: Initializing H.264 encoders...");
    
    // Инициализируем кодировщик для экрана (высокое качество)
    m_screenEncoder = new H264Encoder();
    m_screenEncoder->setPerformanceMode("balanced");   // Сбалансированный режим для качества
    m_screenEncoder->setQualityParameters(H264_QUALITY_MIN_QP_SCREEN, H264_QUALITY_MAX_QP_SCREEN, H264_BITRATE_SCREEN);
    // Инициализируем с аппаратным ускорением, fallback к software
    if (!m_screenEncoder->initializeWithHardware(
        H264_SCREEN_WIDTH, H264_SCREEN_HEIGHT,
        H264_BITRATE_SCREEN, H264_SCREEN_FPS, H264_KEYINT)) {
        LOG_ERROR("FrameProcessor: Screen H.264 encoder initialization FAILED!");
        delete m_screenEncoder;
        m_screenEncoder = nullptr;
    } else {
        const char* accelType = m_screenEncoder->isHardwareAccelerated() ? "HARDWARE" : "SOFTWARE";
        LOG_INFO("FrameProcessor: Screen H.264 encoder initialized successfully ({} ACCELERATED - QP 20-30, 1.5 Mbps, 20 FPS - HIGH QUALITY)", accelType);
    }
    
    // Инициализируем кодировщик для камеры (720p с аппаратным ускорением)
    m_cameraEncoder = new H264Encoder();
    m_cameraEncoder->setPerformanceMode("fast");      // Быстрый режим для камеры
    m_cameraEncoder->setQualityParameters(H264_QUALITY_MIN_QP_CAMERA, H264_QUALITY_MAX_QP_CAMERA, H264_BITRATE_CAMERA);
    // Для камеры всегда используем 720p с аппаратным ускорением
    if (!m_cameraEncoder->initializeWithHardware(
        H264_CAMERA_WIDTH, H264_CAMERA_HEIGHT,
        H264_BITRATE_CAMERA, H264_CAMERA_FPS, H264_KEYINT)) {
        LOG_ERROR("FrameProcessor: Camera H.264 encoder initialization FAILED!");
        delete m_cameraEncoder;
        m_cameraEncoder = nullptr;
    } else {
        const char* accelType = m_cameraEncoder->isHardwareAccelerated() ? "HARDWARE" : "SOFTWARE";
        LOG_INFO("FrameProcessor: Camera H.264 encoder initialized successfully ({} ACCELERATED - QP 22-28, 1.2 Mbps, 20 FPS, 720p)", accelType);
    }
}

FrameProcessor::~FrameProcessor()
{
    if (m_screenEncoder) {
        delete m_screenEncoder;
        m_screenEncoder = nullptr;
    }
    if (m_cameraEncoder) {
        delete m_cameraEncoder;
        m_cameraEncoder = nullptr;
    }
}

void FrameProcessor::processVideoFrame(const QVideoFrame& frame)
{
    if (!m_isCameraActive) {
        m_isCameraActive = true;
        updateBitratesForSimultaneousStreaming();
    }
    
    QVideoFrame clonedFrame = frame;
    if (!clonedFrame.map(QVideoFrame::ReadOnly)) return;
    
    QPixmap pixmap;
    try
    {
        pixmap = videoFrameToPixmap(clonedFrame);
    }
    catch (const std::exception& e)
    {
        clonedFrame.unmap();
        emit processingError(QString("Failed to convert video frame: %1").arg(e.what()));
        return;
    }

    clonedFrame.unmap();

    if (!pixmap.isNull() && pixmap.width() > 0 && pixmap.height() > 0)
    {
        try
        {
            QPixmap croppedPixmap = cropToHorizontal(pixmap);

            if (!croppedPixmap.isNull() && croppedPixmap.width() > 0 && croppedPixmap.height() > 0)
            {
                // Для камеры всегда используем 720p
                QSize targetSize = QSize(H264_CAMERA_WIDTH, H264_CAMERA_HEIGHT);
                std::vector<unsigned char> imageData;
                
                if (m_cameraEncoder && m_cameraEncoder->isInitialized()) {
                    imageData = pixmapToH264(croppedPixmap, targetSize, false); // false = camera
                } else {
                    // H.264 camera encoder not available - cannot process frame
                    return;
                }

                emit cameraFrameProcessed(croppedPixmap, imageData);
            }
        }
        catch (const std::exception& e)
        {
            emit processingError(QString("Failed to process video frame: %1").arg(e.what()));
        }
    }
}

void FrameProcessor::processPixmap(const QPixmap& pixmap)
{
    if (!m_isScreenActive) {
        m_isScreenActive = true;
        updateBitratesForSimultaneousStreaming();
    }
    
    if (pixmap.isNull())
    {
        return;
    }

    try
    {
        if (pixmap.width() > 0 && pixmap.height() > 0)
        {
            // Для экрана используем адаптивный размер (будет определен внутри pixmapToH264)
            QSize targetSize = QSize(-1, -1); // -1 означает "авто-определение"
            std::vector<unsigned char> imageData;
            
            if (m_screenEncoder && m_screenEncoder->isInitialized()) {
                // Для экрана передаем изображение как есть, без обрезки
                imageData = pixmapToH264(pixmap, targetSize, true); // true = screen
            } else {
                // H.264 screen encoder not available - cannot process frame
                return;
            }
            
            emit screenFrameProcessed(pixmap, imageData);
        }
    }
    catch (const std::exception& e)
    {
        emit processingError(QString("Failed to process pixmap: %1").arg(e.what()));
    }
}

QPixmap FrameProcessor::videoFrameToPixmap(const QVideoFrame& frame)
{
    QImage image = frame.toImage();

    if (!image.isNull())
    {
        return QPixmap::fromImage(image);
    }

    QVideoFrame localFrame = frame;
    if (!localFrame.map(QVideoFrame::ReadOnly))
    {
        return QPixmap();
    }

    QImage convertedImage;
    QImage::Format format = QVideoFrameFormat::imageFormatFromPixelFormat(localFrame.pixelFormat());
    if (format == QImage::Format_Invalid)
    {
        format = QImage::Format_RGB32;
    }

    convertedImage = QImage(localFrame.bits(0),
        localFrame.width(),
        localFrame.height(),
        localFrame.bytesPerLine(0),
        format);

    localFrame.unmap();

    return QPixmap::fromImage(convertedImage);
}

QPixmap FrameProcessor::cropToHorizontal(const QPixmap& pixmap)
{
    if (pixmap.isNull()) return pixmap;

    int w = pixmap.width();
    int h = pixmap.height();

    const double targetAspectRatio = 16.0 / 9.0;
    const double currentAspectRatio = static_cast<double>(w) / h;

    if (qAbs(currentAspectRatio - targetAspectRatio) < 0.01)
    {
        return pixmap;
    }

    int cropW = w;
    int cropH = h;

    if (currentAspectRatio > targetAspectRatio)
    {
        cropW = static_cast<int>(h * targetAspectRatio);
        QRect cropRect(0, 0, cropW, cropH);
        return pixmap.copy(cropRect);
    }
    else
    {
        cropH = static_cast<int>(w / targetAspectRatio);
        QRect cropRect(0, 0, cropW, cropH);
        return pixmap.copy(cropRect);
    }
}

std::vector<unsigned char> FrameProcessor::pixmapToH264(const QPixmap& pixmap, QSize targetSize, bool isScreen)
{
    H264Encoder* encoder = isScreen ? m_screenEncoder : m_cameraEncoder;
    
    if (!encoder || !encoder->isInitialized()) {
        // H.264 encoder not available - return empty data
        LOG_ERROR("FrameProcessor: {} encoder not available", isScreen ? "Screen" : "Camera");
        return {};
    }
    
    QImage image = pixmap.toImage();
    
    // Use reusable image buffer to avoid memory allocations
    if (m_reusableImage.size() != image.size() || m_reusableImage.format() != image.format()) {
        m_reusableImage = QImage(image.size(), image.format());
    }
    
    // Copy image data to reusable buffer
    if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32)
    {
        m_reusableImage = image.convertToFormat(QImage::Format_RGB32);
    } else {
        m_reusableImage = image.copy();
    }
    
    // Определяем оптимальный размер с учетом ориентации изображения
    QSize originalSize = m_reusableImage.size();
    QSize adaptedSize;
    
    if (targetSize.width() == -1 || targetSize.height() == -1) {
        // Авто-определение размера (для экрана) - высокое качество
        if (originalSize.width() > originalSize.height()) {
            // Горизонтальное изображение - используем 1920x1080 для высокого качества
            adaptedSize = QSize(1920, 1080);
        } else if (originalSize.width() < originalSize.height()) {
            // Вертикальное изображение - используем 1080x1920
            adaptedSize = QSize(1080, 1920);
        } else {
            // Квадратное изображение - используем 1080x1080
            adaptedSize = QSize(1080, 1080);
        }
        
        // Если изображение маленькое, используем умеренный размер
        if (originalSize.width() < 1280 || originalSize.height() < 720) {
            if (originalSize.width() > originalSize.height()) {
                adaptedSize = QSize(1280, 720);  // 720p для горизонтальных
            } else {
                adaptedSize = QSize(720, 1280);  // 720p для вертикальных
            }
        }
    } else {
        // Используем переданный размер (для камеры - 720p)
        adaptedSize = targetSize;
    }
    
    // Масштабируем с заполнением всего пространства (без черных полей)
    // Use reusable image for scaling to avoid allocations
    if (m_reusableImage.size() != adaptedSize) {
        m_reusableImage = m_reusableImage.scaled(adaptedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    
    // Convert to RGB888 for H.264 encoding
    QImage rgbImage = m_reusableImage.convertToFormat(QImage::Format_RGB888);
    
    // Для экрана переинициализируем кодировщик если размер изменился
    if (isScreen && m_screenEncoder) {
        int currentWidth = rgbImage.width();
        int currentHeight = rgbImage.height();
        
        // Проверяем нужно ли переинициализировать кодировщик
        QSize currentEncoderSize = m_screenEncoder->getCurrentSize();
        if (currentEncoderSize.width() != currentWidth || currentEncoderSize.height() != currentHeight) {
            LOG_INFO("FrameProcessor: Reinitializing screen encoder from {}x{} to {}x{}", 
                     currentEncoderSize.width(), currentEncoderSize.height(),
                     currentWidth, currentHeight);
            
            m_screenEncoder->cleanup();
            if (!m_screenEncoder->initialize(
                currentWidth, currentHeight,
                H264_BITRATE_SCREEN, H264_SCREEN_FPS, H264_KEYINT)) {
                LOG_ERROR("FrameProcessor: Failed to reinitialize screen encoder");
                return {};
            }
        }
    }
    
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("FrameProcessor: About to encode {} frame - size: {}x{}, bytes: {}", 
    //           isScreen ? "screen" : "camera", rgbImage.width(), rgbImage.height(), rgbImage.sizeInBytes());
    
    // Check if encoder is ready
    if (!encoder) {
        LOG_ERROR("FrameProcessor: {} encoder is null!", isScreen ? "Screen" : "Camera");
        return {};
    }
    
    // Encode to H.264 using reusable buffer
    std::vector<unsigned char> h264Data = encoder->encode(rgbImage);
    
    // Убираем спам логов о размере каждого кадра
    // std::stringstream ss;
    // ss << "H.264 encoded: " << h264Data.size() << " bytes | "
    //     << std::fixed << std::setprecision(2)
    //     << (h264Data.size() / 1024.0) << " KB | "
    //     << (h264Data.size() / (1024.0 * 1024.0)) << " MB";
    // LOG_INFO("{}", ss.str());

    if (h264Data.empty()) {
        // H.264 encoding failed - return empty data
        return {};
    }
    
    // Return raw H.264 data
    return h264Data;
}

void FrameProcessor::updateBitratesForSimultaneousStreaming()
{
    bool bothActive = m_isCameraActive && m_isScreenActive;
    
    if (bothActive) {
        LOG_INFO("FrameProcessor: Both streams active - reducing bitrates for performance");
        
        if (m_cameraEncoder && m_cameraEncoder->isInitialized()) {
            m_cameraEncoder->setQualityParameters(H264_QUALITY_MIN_QP_CAMERA, H264_QUALITY_MAX_QP_CAMERA, H264_BITRATE_CAMERA_SIMULTANEOUS);
        }
        
        if (m_screenEncoder && m_screenEncoder->isInitialized()) {
            m_screenEncoder->setQualityParameters(H264_QUALITY_MIN_QP_SCREEN, H264_QUALITY_MAX_QP_SCREEN, H264_BITRATE_SCREEN_SIMULTANEOUS);
        }
    } else {
        LOG_INFO("FrameProcessor: Single stream active - using normal bitrates");
        
        if (m_cameraEncoder && m_cameraEncoder->isInitialized()) {
            m_cameraEncoder->setQualityParameters(H264_QUALITY_MIN_QP_CAMERA, H264_QUALITY_MAX_QP_CAMERA, H264_BITRATE_CAMERA);
        }
        
        if (m_screenEncoder && m_screenEncoder->isInitialized()) {
            m_screenEncoder->setQualityParameters(H264_QUALITY_MIN_QP_SCREEN, H264_QUALITY_MAX_QP_SCREEN, H264_BITRATE_SCREEN);
        }
    }
}

void FrameProcessor::drawCursorOnPixmap(QPixmap& pixmap, const QPoint& cursorPos, const QRect& screenGeometry)
{
    if (pixmap.isNull() || !screenGeometry.contains(cursorPos))
    {
        return;
    }

    QPoint screenRelativePos = cursorPos - screenGeometry.topLeft();
    
#ifdef Q_OS_WIN
    CURSORINFO cursorInfo = { 0 };
    cursorInfo.cbSize = sizeof(CURSORINFO);
    if (!GetCursorInfo(&cursorInfo) || cursorInfo.flags != CURSOR_SHOWING)
    {
        return;
    }

    // Создаем копию курсора, чтобы не влиять на оригинальный курсор
    HCURSOR hCursorCopy = CopyCursor(cursorInfo.hCursor);
    if (!hCursorCopy)
    {
        return;
    }

    ICONINFO iconInfo;
    if (!GetIconInfo(hCursorCopy, &iconInfo))
    {
        DestroyCursor(hCursorCopy);
        return;
    }

    QPoint hotspot(iconInfo.xHotspot, iconInfo.yHotspot);
    
    int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
    int cursorHeight = GetSystemMetrics(SM_CYCURSOR);
    
    if (cursorWidth == 0) cursorWidth = 32;
    if (cursorHeight == 0) cursorHeight = 32;
    
    if (iconInfo.hbmColor)
    {
        BITMAP bmp;
        GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);
        if (bmp.bmWidth > 0 && bmp.bmHeight > 0)
        {
            cursorWidth = bmp.bmWidth;
            cursorHeight = bmp.bmHeight;
        }
    }
    else if (iconInfo.hbmMask)
    {
        BITMAP bmp;
        GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp);
        if (bmp.bmWidth > 0 && bmp.bmHeight > 1)
        {
            cursorWidth = bmp.bmWidth;
            cursorHeight = bmp.bmHeight / 2;
        }
    }
    
    QImage cursorImage(cursorWidth, cursorHeight, QImage::Format_ARGB32);
    cursorImage.fill(Qt::transparent);
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    if (hdcMem)
    {
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cursorWidth;
        bmi.bmiHeader.biHeight = -cursorHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        void* bitsWhite = nullptr;
        void* bitsBlack = nullptr;
        HBITMAP hbitmapWhite = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bitsWhite, NULL, 0);
        HBITMAP hbitmapBlack = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &bitsBlack, NULL, 0);
        
        if (hbitmapWhite && hbitmapBlack)
        {
            uint32_t whiteColor = 0xFFFFFFFF;
            uint32_t blackColor = 0xFF000000;
            uint32_t* whiteBits = reinterpret_cast<uint32_t*>(bitsWhite);
            uint32_t* blackBits = reinterpret_cast<uint32_t*>(bitsBlack);
            const int pixelCount = cursorWidth * cursorHeight;
            
            for (int i = 0; i < pixelCount; ++i)
            {
                whiteBits[i] = whiteColor;
                blackBits[i] = blackColor;
            }
            
            HGDIOBJ oldObj = SelectObject(hdcMem, hbitmapWhite);
            DrawIconEx(hdcMem, 0, 0, hCursorCopy, 0, 0, 0, NULL, DI_NORMAL);
            GdiFlush();
            SelectObject(hdcMem, hbitmapBlack);
            DrawIconEx(hdcMem, 0, 0, hCursorCopy, 0, 0, 0, NULL, DI_NORMAL);
            GdiFlush();
            
            uint32_t* cursorBits = reinterpret_cast<uint32_t*>(cursorImage.bits());
            
            for (int i = 0; i < pixelCount; ++i)
            {
                uint32_t whiteBgra = whiteBits[i];
                uint32_t blackBgra = blackBits[i];
                
                uint8_t whiteB = (whiteBgra >> 0) & 0xFF;
                uint8_t whiteG = (whiteBgra >> 8) & 0xFF;
                uint8_t whiteR = (whiteBgra >> 16) & 0xFF;
                
                uint8_t blackB = (blackBgra >> 0) & 0xFF;
                uint8_t blackG = (blackBgra >> 8) & 0xFF;
                uint8_t blackR = (blackBgra >> 16) & 0xFF;
                
                bool visibleOnWhite = (whiteR != 255 || whiteG != 255 || whiteB != 255);
                bool visibleOnBlack = (blackR != 0 || blackG != 0 || blackB != 0);
                
                if (visibleOnBlack && visibleOnWhite)
                {
                    cursorBits[i] = qRgba(blackR, blackG, blackB, 255);
                }
                else if (visibleOnBlack)
                {
                    cursorBits[i] = qRgba(blackR, blackG, blackB, 255);
                }
                else if (visibleOnWhite)
                {
                    cursorBits[i] = qRgba(whiteR, whiteG, whiteB, 255);
                }
            }
            
            SelectObject(hdcMem, oldObj);
            DeleteObject(hbitmapWhite);
            DeleteObject(hbitmapBlack);
        }
        
        DeleteDC(hdcMem);
    }
    
    ReleaseDC(NULL, hdcScreen);
    
    // Удаляем копию курсора после использования
    DestroyCursor(hCursorCopy);
    
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    
    if (!cursorImage.isNull())
    {
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(
            screenRelativePos.x() - hotspot.x(),
            screenRelativePos.y() - hotspot.y(),
            cursorImage
        );
    }
#else
    QCursor cursor;
    QPixmap cursorPixmap = cursor.pixmap();
    if (!cursorPixmap.isNull())
    {
        QPoint hotspot = cursor.hotSpot();
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawPixmap(
            screenRelativePos.x() - hotspot.x(),
            screenRelativePos.y() - hotspot.y(),
            cursorPixmap
        );
    }
#endif
}

