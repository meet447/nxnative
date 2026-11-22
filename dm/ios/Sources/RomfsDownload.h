// Download Documents/romfs.img from a VPS URL baked into a slim IPA (romfs.url + DM_COPY_ROMFS=NO).
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface RomfsDownload : NSObject
/// URL from <App>/romfs.url, or nil when this IPA does not offer a download.
+ (nullable NSString *)bundledURL;
/// Expected byte size from <App>/romfs.id (size=…), or 0 if unknown.
+ (uint64_t)expectedSize;
/// Start (or resume) the download into Documents/romfs.img.part → romfs.img.
/// progress: 0…1 (or -1 when total unknown). done: error nil on success.
+ (void)startWithProgress:(void (^)(double progress, int64_t got, int64_t total))progress
                   completion:(void (^)(NSError *_Nullable error))done;
+ (void)cancel;
@end

NS_ASSUME_NONNULL_END
