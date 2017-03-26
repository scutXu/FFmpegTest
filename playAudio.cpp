extern "C" {
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include "RingBuffer.h"
#include <OpenAL/OpenAL.h>
#include <thread>


using std::thread;

AVFormatContext * formatCtx = NULL;
const char * fileName = "/Users/zj-db0731/Downloads/testVideos/480x854.m4v";
AVCodecContext * audioDecCtx = NULL;
int audioStreamIndex;
AVFrame *frame = NULL;



ALCdevice * alDev = NULL;
ALCcontext * alCtx = NULL;
ALuint alSource;
ALuint alBuffers[3];
ALenum alFormat;

void * audioFrame = NULL;
int audioFrameSize;

#define TARGET_SAMPLE_RATE 44100
#define TARGET_CHANNEL_LAYOUT AV_CH_LAYOUT_MONO
#define TARGET_SAMPLE_FORMAT AV_SAMPLE_FMT_S16
#define MAX_SAMPLES_IN_TARGET_BUFFER TARGET_SAMPLE_RATE //1秒

struct SwrContext *swr_ctx = NULL;
int targetChannelCount = 0;
int targetNumBytePerSample = 0;
void * targetAudioBuffer = NULL;
int targetAudioBufferSize = 0;

RingBuffer * rBuf = NULL;



void initFFmpeg()
{
    av_register_all();
    if (avformat_open_input(&formatCtx, fileName, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", fileName);
        exit(1);
    }
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    
    
    
    audioStreamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audioStreamIndex < 0) {
        fprintf(stderr, "audio stream not found\n");
        exit(1);
    }
    
    AVStream * audioStream = formatCtx->streams[audioStreamIndex];
    AVCodec * dec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    if(!dec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    
    audioDecCtx = avcodec_alloc_context3(NULL);
    if(audioDecCtx == NULL) {
        fprintf(stderr, "alloc codec_context return null\n");
        exit(1);
    }
    
    if (avcodec_parameters_to_context(audioDecCtx, audioStream->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        exit(1);
    }
    
    AVDictionary *opts = NULL;
    int ret = av_dict_set(&opts, "refcounted_frames", "0",0);
    if(ret < 0) {
        printf("av_dict_set_refcounted_frames failed");
        exit(1);
    }
    if (avcodec_open2(audioDecCtx, dec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        exit(1);
    }
    
    
    
}

void audioLoop()
{
    for(int i=0;i<3;++i) {
        rBuf->read((unsigned char *)audioFrame, 0, audioFrameSize);
        alBufferData(alBuffers[0], alFormat, audioFrame, audioFrameSize, TARGET_SAMPLE_RATE);
    }
    alSourceQueueBuffers(alSource, 3, alBuffers);
    alSourcePlay(alSource);
    
    ALuint unqueuedBuffer;
    ALint val;
    while(true) {
        alGetSourcei(alSource, AL_BUFFERS_PROCESSED, &val);
        while(val--) {
            alSourceUnqueueBuffers(alSource, 1, &unqueuedBuffer);
            rBuf->read((unsigned char *)audioFrame, 0, audioFrameSize);
            alBufferData(unqueuedBuffer, alFormat, audioFrame, audioFrameSize, TARGET_SAMPLE_RATE);
        }
        
    }
}

void initAL()
{
    if(av_sample_fmt_is_planar(TARGET_SAMPLE_FORMAT)) {
        printf("openal do not support planar data");
        exit(1);
    }
    
    alDev = alcOpenDevice(NULL);
    if(!alDev) {
        printf("alcOpenDevice failed");
    }
    
    alCtx = alcCreateContext(alDev, NULL);
    alcMakeContextCurrent(alCtx);
    if(!alCtx) {
        printf("alcMakeContextCurrent failed");
    }
    
    alGenBuffers(3, alBuffers);
    alGenSources(1, &alSource);
    if(alGetError() != AL_NO_ERROR) {
        printf("alGenBuffers\alGenSources failed");
    }
    
    
    if(targetNumBytePerSample == 1) {
        if(targetChannelCount == 1) {
            alFormat = AL_FORMAT_MONO8;
        }
        else if(targetChannelCount == 2) {
            alFormat = AL_FORMAT_STEREO8;
        }
    }
    else if(targetNumBytePerSample ==2) {
        if(targetChannelCount == 1) {
            alFormat = AL_FORMAT_MONO16;
        }
        else if(targetChannelCount == 2) {
            alFormat = AL_FORMAT_STEREO16;
        }
    }
    else {
        printf("unsupported sample format");
        exit(1);
    }
    if(!alFormat) {
        printf("invalid al format");
        exit(1);
    }
    
    thread t(audioLoop);
    t.detach();
}

void releaseAL()
{
}


int main()
{
    rBuf = new RingBuffer(4096,RingBuffer::READ_MODE_BLOCK,RingBuffer::WRITE_MODE_BLOCK);
    
    targetChannelCount = av_get_channel_layout_nb_channels(TARGET_CHANNEL_LAYOUT);
    targetNumBytePerSample = av_get_bytes_per_sample(TARGET_SAMPLE_FORMAT);
    targetAudioBufferSize = MAX_SAMPLES_IN_TARGET_BUFFER * targetChannelCount * targetNumBytePerSample;
    targetAudioBuffer = malloc(targetAudioBufferSize);
    
    
    initFFmpeg();
    
    int sampleRate = audioDecCtx->sample_rate;
    int numChannel = audioDecCtx->channels;
    AVSampleFormat sampleFormat = audioDecCtx->sample_fmt;
    
    
    if(sampleRate != TARGET_SAMPLE_RATE ||
       audioDecCtx->sample_fmt != TARGET_SAMPLE_FORMAT ||
       audioDecCtx->channels != targetChannelCount) {
        swr_ctx = swr_alloc();
        if(!swr_ctx) {
            printf("Could not allocate resampler context\n");
            exit(1);
        }
        av_opt_set_int(swr_ctx,"in_channel_layout",av_get_default_channel_layout(numChannel),0);
        av_opt_set_int(swr_ctx, "in_sample_rate", sampleRate, 0);
        av_opt_set_int(swr_ctx,"in_sample_fmt",sampleFormat,0);
        
        av_opt_set_int(swr_ctx, "out_channel_layout",TARGET_CHANNEL_LAYOUT,0);
        av_opt_set_int(swr_ctx, "out_sample_rate",TARGET_SAMPLE_RATE, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",TARGET_SAMPLE_FORMAT, 0);
        
        if(swr_init(swr_ctx) < 0) {
            printf("Failed to initialize the resampling context\n");
            exit(1);
        }
    }
    
    audioFrameSize = TARGET_SAMPLE_RATE * av_get_bytes_per_sample(TARGET_SAMPLE_FORMAT) * targetChannelCount;
    audioFrame = malloc(audioFrameSize);
    
    initAL();
    
    
    int got_frame = 0;
    
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    frame = av_frame_alloc();
    
    while(av_read_frame(formatCtx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        if(pkt.stream_index != audioStreamIndex) {
            av_packet_unref(&orig_pkt);
            continue;
        }
        
        do {
            got_frame = 0;
            int ret;
            if((ret = avcodec_decode_audio4(audioDecCtx, frame, &got_frame, &pkt)) < 0) {
                printf("avcodec_decode_audio4 failed:%s\n",av_err2str(ret));
                break;
            }
            ret = FFMIN(ret, pkt.size);
            pkt.data += ret;
            pkt.size -= ret;
            if(got_frame) {
                if(swr_ctx != NULL) {
                    if(frame->nb_samples > MAX_SAMPLES_IN_TARGET_BUFFER) {
                        printf("target buffer too small");
                    }
                    
                    int ret = swr_convert(swr_ctx,(uint8_t **)(&targetAudioBuffer),MAX_SAMPLES_IN_TARGET_BUFFER,(const uint8_t **)(frame->data),frame->nb_samples);
                    
                    if(ret < 0) {
                        printf("swr_convert failed\n");
                        exit(1);
                    }
                    int size = av_samples_get_buffer_size(NULL,targetChannelCount,frame->nb_samples,TARGET_SAMPLE_FORMAT,1);
                    if(size < 0) {
                        printf("av_samples_get_buffer_size failed\n");
                        exit(1);
                    }
                    rBuf->write((unsigned char *)targetAudioBuffer, 0, size);
                }
                else {
                    uint8_t * data = frame->data[0];
                    int size = frame->linesize[0];
                    rBuf->write(data, 0, size);
                }
            }
            
        }while(pkt.size > 0);
        av_packet_unref(&orig_pkt);
    }
    
    
    
}
