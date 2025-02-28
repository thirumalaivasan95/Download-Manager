#ifndef MEDIA_HANDLER_H
#define MEDIA_HANDLER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <mutex>

namespace dm {
namespace core {

// Forward declarations
class DownloadManager;
class DownloadTask;

/**
 * @brief Media type enumeration
 */
enum class MediaType {
    UNKNOWN,
    VIDEO,
    AUDIO,
    IMAGE,
    DOCUMENT,
    ARCHIVE,
    EXECUTABLE,
    TEXT
};

/**
 * @brief Media format structure
 */
struct MediaFormat {
    std::string name;         // Format name
    std::string description;  // Format description
    MediaType type;           // Media type
    std::vector<std::string> extensions; // File extensions
    std::vector<std::string> mimeTypes;  // MIME types
    bool supportsStreaming;   // Whether streaming is supported
    bool supportsExtraction;  // Whether content extraction is supported
    bool supportsPreview;     // Whether preview is supported
};

/**
 * @brief Media information structure
 */
struct MediaInfo {
    std::string filePath;     // File path
    std::string url;          // Source URL
    MediaType type;           // Media type
    MediaFormat format;       // Media format
    int64_t fileSize;         // File size in bytes
    std::string resolution;   // Resolution for video/image
    std::string duration;     // Duration for audio/video
    std::string bitrate;      // Bitrate for audio/video
    std::string codec;        // Codec information
    std::vector<std::string> metadata; // Additional metadata
};

/**
 * @brief Media extraction callback type
 */
using ExtractionCallback = std::function<void(const std::string& outputPath, bool success)>;

/**
 * @brief Media preview callback type
 */
using PreviewCallback = std::function<void(const std::string& previewPath, bool success)>;

/**
 * @brief Media handler class
 * 
 * Handles media files with specialized processing
 */
class MediaHandler {
public:
    /**
     * @brief Construct a new MediaHandler
     * 
     * @param downloadManager Reference to download manager
     */
    explicit MediaHandler(DownloadManager& downloadManager);
    
    /**
     * @brief Destroy the MediaHandler
     */
    ~MediaHandler();
    
    /**
     * @brief Initialize the media handler
     * 
     * @return true if successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Shutdown the media handler
     */
    void shutdown();
    
    /**
     * @brief Detect media type from file
     * 
     * @param filePath Path to the file
     * @return MediaType The detected media type
     */
    MediaType detectMediaType(const std::string& filePath);
    
    /**
     * @brief Detect media type from URL and content type
     * 
     * @param url The URL
     * @param contentType The content type (MIME type)
     * @return MediaType The detected media type
     */
    MediaType detectMediaTypeFromUrl(const std::string& url, const std::string& contentType);
    
    /**
     * @brief Get media format for a file
     * 
     * @param filePath Path to the file
     * @return MediaFormat The media format
     */
    MediaFormat getMediaFormat(const std::string& filePath);
    
    /**
     * @brief Get detailed media information
     * 
     * @param filePath Path to the file
     * @return MediaInfo The media information
     */
    MediaInfo getMediaInfo(const std::string& filePath);
    
    /**
     * @brief Extract content from archive file
     * 
     * @param archivePath Path to the archive file
     * @param outputPath Path to extract to
     * @param callback Optional callback for completion
     * @return true if extraction started successfully, false otherwise
     */
    bool extractArchive(const std::string& archivePath, 
                       const std::string& outputPath,
                       ExtractionCallback callback = nullptr);
    
    /**
     * @brief Generate a preview for a media file
     * 
     * @param filePath Path to the media file
     * @param outputPath Path to save the preview
     * @param callback Optional callback for completion
     * @return true if preview generation started successfully, false otherwise
     */
    bool generatePreview(const std::string& filePath, 
                        const std::string& outputPath,
                        PreviewCallback callback = nullptr);
    
    /**
     * @brief Get a thumbnail for a media file
     * 
     * @param filePath Path to the media file
     * @param outputPath Path to save the thumbnail
     * @param width Thumbnail width
     * @param height Thumbnail height
     * @return true if successful, false otherwise
     */
    bool generateThumbnail(const std::string& filePath, 
                          const std::string& outputPath,
                          int width = 128, 
                          int height = 128);
    
    /**
     * @brief Convert a media file to another format
     * 
     * @param inputPath Path to the input file
     * @param outputPath Path to save the converted file
     * @param targetFormat Target format extension (e.g., "mp4")
     * @return true if conversion started successfully, false otherwise
     */
    bool convertMedia(const std::string& inputPath, 
                     const std::string& outputPath,
                     const std::string& targetFormat);
    
    /**
     * @brief Register a custom handler for a media type
     * 
     * @param type The media type
     * @param handler The handler function
     */
    void registerCustomHandler(MediaType type, 
                              std::function<void(const std::string& filePath)> handler);
    
    /**
     * @brief Register a custom format detector
     * 
     * @param detector The detector function
     */
    void registerFormatDetector(std::function<MediaFormat(const std::string& filePath)> detector);
    
    /**
     * @brief Get all supported formats
     * 
     * @return std::vector<MediaFormat> The supported formats
     */
    std::vector<MediaFormat> getSupportedFormats() const;
    
    /**
     * @brief Check if a format is supported
     * 
     * @param extension The file extension
     * @return true if supported, false otherwise
     */
    bool isFormatSupported(const std::string& extension) const;
    
    /**
     * @brief Get the format for a file extension
     * 
     * @param extension The file extension
     * @return MediaFormat The media format
     */
    MediaFormat getFormatForExtension(const std::string& extension) const;
    
    /**
     * @brief Get the format for a MIME type
     * 
     * @param mimeType The MIME type
     * @return MediaFormat The media format
     */
    MediaFormat getFormatForMimeType(const std::string& mimeType) const;
    
    /**
     * @brief Notify the media handler of a completed download
     * 
     * @param task The download task
     */
    void onDownloadCompleted(std::shared_ptr<DownloadTask> task);
    
    /**
     * @brief Set the temporary directory
     * 
     * @param directory The directory path
     */
    void setTempDirectory(const std::string& directory);
    
    /**
     * @brief Get the temporary directory
     * 
     * @return std::string The directory path
     */
    std::string getTempDirectory() const;
    
    /**
     * @brief Enable or disable automatic processing of downloads
     * 
     * @param enable True to enable, false to disable
     */
    void setAutoProcessing(bool enable);
    
    /**
     * @brief Check if automatic processing is enabled
     * 
     * @return true if enabled, false otherwise
     */
    bool isAutoProcessingEnabled() const;
    
private:
    /**
     * @brief Initialize the list of supported formats
     */
    void initializeSupportedFormats();
    
    /**
     * @brief Detect format from file content
     * 
     * @param filePath Path to the file
     * @return MediaFormat The detected format
     */
    MediaFormat detectFormatFromContent(const std::string& filePath);
    
    /**
     * @brief Check if a file is an image
     * 
     * @param filePath Path to the file
     * @return true if an image, false otherwise
     */
    bool isImage(const std::string& filePath);
    
    /**
     * @brief Check if a file is an audio file
     * 
     * @param filePath Path to the file
     * @return true if an audio file, false otherwise
     */
    bool isAudio(const std::string& filePath);
    
    /**
     * @brief Check if a file is a video file
     * 
     * @param filePath Path to the file
     * @return true if a video file, false otherwise
     */
    bool isVideo(const std::string& filePath);
    
    /**
     * @brief Check if a file is a document
     * 
     * @param filePath Path to the file
     * @return true if a document, false otherwise
     */
    bool isDocument(const std::string& filePath);
    
    /**
     * @brief Check if a file is an archive
     * 
     * @param filePath Path to the file
     * @return true if an archive, false otherwise
     */
    bool isArchive(const std::string& filePath);
    
    // Member variables
    DownloadManager& downloadManager_;
    std::vector<MediaFormat> supportedFormats_;
    std::map<MediaType, std::function<void(const std::string& filePath)>> customHandlers_;
    std::vector<std::function<MediaFormat(const std::string& filePath)>> formatDetectors_;
    std::string tempDirectory_;
    std::atomic<bool> autoProcessingEnabled_;
    std::mutex mutex_;
};

} // namespace core
} // namespace dm

#endif // MEDIA_HANDLER_H