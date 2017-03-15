#include <libavformat/avformat.h>


AVFormatContext * formatCtx = NULL;
const char * fileName = "/Users/zj-db0731/Downloads/testVideos/test2.mp4";
AVCodecContext * audioDecCtx = NULL;
int audioStreamIndex;
AVFrame *frame = NULL;
AVPacket pkt;

enum AVSampleFormat audioSampleFormat;
int numChannel;


int main()
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
    AVCodec * dec = avcodec_find_decoder(audioStram->codecpar->codec_id);
    if(!dec) {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }
    
    if (avcodec_parameters_to_context(audioDecCtx, audioStram->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        exit(1);
    }
    
    if (avcodec_open2(audioDecCtx, dec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        exit(1);
    }
    
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
               size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
            }
            
        }while(pkt.size > 0);
        av_packet_unref(&orig_pkt);
    }
    
}
