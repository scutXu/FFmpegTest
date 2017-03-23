extern "C" {
#include <libavformat/avformat.h>
}

#include "RingBuffer.h"
#include <OpenAL/OpenAL.h>
#include <thread>


using std::thread;

AVFormatContext * formatCtx = NULL;
const char * fileName = "/Users/zj-db0731/Downloads/testVideos/test2.mp4";
AVCodecContext * audioDecCtx = NULL;
int audioStreamIndex;
AVFrame *frame = NULL;
AVPacket pkt;
int numBytePerSample;
int numChannel;
int sampleRate;


ALCdevice * alDev;
ALCcontext * alCtx;
ALuint alSource;
ALuint alBuffers[3];
ALenum alFormat;

void * audioFrame;
int audioFrameSize;

RingBuffer * rBuf;

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
    
    audioDecCtx = avcodec_alloc_context3(NULL);
    if(audioDecCtx == NULL) {
        fprintf(stderr, "alloc codec_context return null\n");
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
    
    if (avcodec_parameters_to_context(audioDecCtx, audioStream->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        exit(1);
    }
    
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "refcounted_frames", 0,0);
    if (avcodec_open2(audioDecCtx, dec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        exit(1);
    }
    
    if(av_sample_fmt_is_planar(audioDecCtx->sample_fmt)) {
        //todo: use libswresample or libavfilter to convert the frame to packed data
        fprintf(stderr, "planar data is not supported at present\n");
        exit(1);
    }
}

void audioLoop()
{
    for(int i=0;i<3;++i) {
        rBuf->read((unsigned char *)audioFrame, 0, audioFrameSize);
        alBufferData(alBuffers[0], alFormat, audioFrame, audioFrameSize, sampleRate);
    }
    alSourceQueueBuffers(alSource, 3, alBuffers);
    alSourcePlay(alSource);
    while(true) {
        
    }
}

void initAL()
{
    alDev = alcOpenDevice(NULL);
    if(!alDev) {
        
    }
    
    alCtx = alcCreateContext(alDev, NULL);
    alcMakeContextCurrent(alCtx);
    if(!alCtx) {
        
    }
    
    alGenBuffers(3, alBuffers);
    alGenSources(1, &alSource);
    if(alGetError() != AL_NO_ERROR) {
        
    }
    
    if(numBytePerSample == 1) {
        if(numChannel == 1) {
            alFormat = AL_FORMAT_MONO8;
        }
        else if(numChannel == 2) {
            alFormat = AL_FORMAT_STEREO8;
        }
    }
    else if(numBytePerSample ==2) {
        if(numChannel == 1) {
            alFormat = AL_FORMAT_MONO16;
        }
        else if(numChannel == 2) {
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
    rBuf = new RingBuffer(2048,RingBuffer::READ_MODE_BLOCK,RingBuffer::WRITE_MODE_BLOCK);

    initFFmpeg();
    
    sampleRate = audioDecCtx->sample_rate;
    numBytePerSample = av_get_bytes_per_sample(audioDecCtx->sample_fmt);
    numChannel = audioDecCtx->channels;
    audioFrameSize = sampleRate * numBytePerSample * numChannel;
    audioFrame = malloc(audioFrameSize);
    
    initAL();
    
    int got_frame = 0;
    while(av_read_frame(formatCtx, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        do {
            got_frame = 0;
            int ret;
            if((ret = avcodec_decode_audio4(audioDecCtx, frame, &got_frame, &pkt)) < 0) {
                
            }
            ret = FFMIN(ret, pkt.size);
            if(got_frame) {
                uint8_t * data = frame->data[0];
                int size = frame->linesize[0];
                
            }
            
        }while(pkt.size > 0);
        av_packet_unref(&orig_pkt);
    }
    
}
