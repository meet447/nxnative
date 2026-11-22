#import "RomfsDownload.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

@interface RomfsDownload () <NSURLSessionDownloadDelegate>
@property (nonatomic) NSURLSession *session;
@property (nonatomic, copy) void (^progress)(double, int64_t, int64_t);
@property (nonatomic, copy) void (^completion)(NSError *_Nullable);
@property (nonatomic) NSURL *partURL;
@property (nonatomic) NSURL *destURL;
@property (nonatomic) int64_t resumeFrom;
@end

@implementation RomfsDownload

static RomfsDownload *g_dl;

+ (NSString *)bundledURL {
    NSString *path = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"romfs.url"];
    NSString *txt = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    NSString *url = [txt stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    return url.length ? url : nil;
}

+ (uint64_t)expectedSize {
    NSString *path = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"romfs.id"];
    NSString *txt = [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
    if (!txt) return 0;
    for (NSString *tok in [txt componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet]) {
        if ([tok hasPrefix:@"size="]) return (uint64_t)strtoull(tok.UTF8String + 5, NULL, 10);
    }
    return 0;
}

+ (NSURL *)documentsURL {
    return [NSURL fileURLWithPath:NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0] isDirectory:YES];
}

+ (void)cancel {
    [g_dl.session invalidateAndCancel];
    g_dl = nil;
}

+ (void)startWithProgress:(void (^)(double, int64_t, int64_t))progress completion:(void (^)(NSError *_Nullable))done {
    [self cancel];
    NSString *urlStr = [self bundledURL];
    if (!urlStr) {
        done([NSError errorWithDomain:@"RomfsDownload" code:1 userInfo:@{NSLocalizedDescriptionKey: @"This app has no romfs.url download address."}]);
        return;
    }
    NSURL *url = [NSURL URLWithString:urlStr];
    if (!url || !url.scheme.length) {
        done([NSError errorWithDomain:@"RomfsDownload" code:2 userInfo:@{NSLocalizedDescriptionKey: @"romfs.url is not a valid URL."}]);
        return;
    }
    uint64_t need = [self expectedSize];
    NSURL *doc = [self documentsURL];
    struct statvfs vfs;
    if (need && !statvfs(doc.fileSystemRepresentation, &vfs)) {
        uint64_t freeb = (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize;
        if (freeb < need + (256ull << 20)) {
            done([NSError errorWithDomain:@"RomfsDownload" code:3 userInfo:@{
                NSLocalizedDescriptionKey: [NSString stringWithFormat:
                    @"Not enough free storage. Need about %.1f GB for the game files; this device has about %.1f GB free.",
                    need / (1024.0 * 1024.0 * 1024.0), freeb / (1024.0 * 1024.0 * 1024.0)]
            }]);
            return;
        }
    }

    RomfsDownload *dl = [RomfsDownload new];
    g_dl = dl;
    dl.progress = progress;
    dl.completion = done;
    dl.destURL = [doc URLByAppendingPathComponent:@"romfs.img"];
    dl.partURL = [doc URLByAppendingPathComponent:@"romfs.img.part"];

    // Resume from a partial file left by a previous attempt
    NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:dl.partURL.path error:nil];
    dl.resumeFrom = attrs ? (int64_t)attrs.fileSize : 0;

    NSURLSessionConfiguration *cfg = [NSURLSessionConfiguration defaultSessionConfiguration];
    cfg.allowsCellularAccess = YES;
    cfg.timeoutIntervalForRequest = 60;
    cfg.timeoutIntervalForResource = 0;   // large RomFS; no overall timeout
    cfg.waitsForConnectivity = YES;
    dl.session = [NSURLSession sessionWithConfiguration:cfg delegate:dl delegateQueue:nil];

    NSMutableURLRequest *req = [NSMutableURLRequest requestWithURL:url];
    if (dl.resumeFrom > 0) {
        [req setValue:[NSString stringWithFormat:@"bytes=%lld-", (long long)dl.resumeFrom] forHTTPHeaderField:@"Range"];
        fprintf(stderr, "romfs download: resume from %lld bytes\n", (long long)dl.resumeFrom);
    } else {
        fprintf(stderr, "romfs download: GET %s\n", urlStr.UTF8String);
    }
    [[dl.session downloadTaskWithRequest:req] resume];
}

- (void)finish:(NSError *)err {
    void (^done)(NSError *) = self.completion;
    self.completion = nil;
    self.progress = nil;
    [self.session finishTasksAndInvalidate];
    if (g_dl == self) g_dl = nil;
    if (done) dispatch_async(dispatch_get_main_queue(), ^{ done(err); });
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)task
                                           didWriteData:(int64_t)written
                                      totalBytesWritten:(int64_t)got
                              totalBytesExpectedToWrite:(int64_t)total {
    int64_t absoluteGot = self.resumeFrom + got;
    int64_t absoluteTotal = total > 0 ? self.resumeFrom + total : (int64_t)[RomfsDownload expectedSize];
    double p = absoluteTotal > 0 ? (double)absoluteGot / (double)absoluteTotal : -1.0;
    void (^prog)(double, int64_t, int64_t) = self.progress;
    if (prog) dispatch_async(dispatch_get_main_queue(), ^{ prog(p, absoluteGot, absoluteTotal); });
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)task didFinishDownloadingToURL:(NSURL *)location {
    NSFileManager *fm = NSFileManager.defaultManager;
    NSError *err = nil;
    NSHTTPURLResponse *http = (NSHTTPURLResponse *)task.response;
    int status = [http isKindOfClass:NSHTTPURLResponse.class] ? (int)http.statusCode : 200;

    if (status == 206 && self.resumeFrom > 0) {
        // Append ranged body onto the partial file
        NSFileHandle *out = [NSFileHandle fileHandleForWritingAtPath:self.partURL.path];
        if (!out) {
            [self finish:[NSError errorWithDomain:@"RomfsDownload" code:4 userInfo:@{NSLocalizedDescriptionKey: @"Could not open the partial download to resume."}]];
            return;
        }
        [out seekToEndOfFile];
        NSFileHandle *in = [NSFileHandle fileHandleForReadingFromURL:location error:&err];
        if (!in) { [out closeFile]; [self finish:err]; return; }
        for (;;) {
            NSData *chunk = [in readDataOfLength:1 << 20];
            if (!chunk.length) break;
            @try { [out writeData:chunk]; }
            @catch (NSException *ex) {
                [in closeFile]; [out closeFile];
                [self finish:[NSError errorWithDomain:@"RomfsDownload" code:4 userInfo:@{NSLocalizedDescriptionKey: ex.reason ?: @"write failed"}]];
                return;
            }
        }
        [in closeFile]; [out closeFile];
    } else if (status >= 200 && status < 300) {
        [fm removeItemAtURL:self.partURL error:nil];
        if (![fm moveItemAtURL:location toURL:self.partURL error:&err]) {
            // Cross-volume: copy
            err = nil;
            [fm removeItemAtURL:self.partURL error:nil];
            if (![fm copyItemAtURL:location toURL:self.partURL error:&err]) { [self finish:err]; return; }
        }
    } else {
        [self finish:[NSError errorWithDomain:@"RomfsDownload" code:status userInfo:@{
            NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Download failed (HTTP %d).", status]
        }]];
        return;
    }

    uint64_t want = [RomfsDownload expectedSize];
    NSDictionary *attrs = [fm attributesOfItemAtPath:self.partURL.path error:nil];
    uint64_t got = attrs.fileSize;
    if (want && got != want) {
        [self finish:[NSError errorWithDomain:@"RomfsDownload" code:5 userInfo:@{
            NSLocalizedDescriptionKey: [NSString stringWithFormat:
                @"Downloaded size (%llu bytes) does not match this build's romfs.id (%llu bytes). Check the VPS file.",
                (unsigned long long)got, (unsigned long long)want]
        }]];
        return;
    }

    [fm removeItemAtURL:self.destURL error:nil];
    // Prefer exact name romfs.img (main.m looks for any letter case, but push_romfs uses romfs.img)
    if (![fm moveItemAtURL:self.partURL toURL:self.destURL error:&err]) { [self finish:err]; return; }
    // Keep large game data out of iCloud backups
    [self.destURL setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
    fprintf(stderr, "romfs download: wrote %s (%llu bytes)\n", self.destURL.fileSystemRepresentation, (unsigned long long)got);
    [self finish:nil];
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    if (error && error.code != NSURLErrorCancelled) [self finish:error];
}

@end
