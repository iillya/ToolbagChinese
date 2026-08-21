#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#import <ATen/ATen.h>
#import <CoreML/CoreML.h>

NS_ASSUME_NONNULL_BEGIN

@interface PTMCoreMLFeatureProvider : NSObject<MLFeatureProvider>

- (instancetype)initWithFeatureNames:(NSSet<NSString*>*)featureNames;

- (void)clearInputTensors;

- (void)setInputTensor:(const at::Tensor&)tensor forFeatureName:(NSString*)name;

@end

NS_ASSUME_NONNULL_END

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
