#include <libavformat/avformat.h>

AVFormatContext * fmt_ctx = NULL;
int videoStreamIndex;
int audioStreamIndex;
int subtitleStreamIndex;
AVCodecContext * video_dec_ctx = NULL;
AVCodecContext * audio_dec_ctx = NULL;
AVCodecContext * subtitle_dec_ctx = NULL;

const char * src_filename = "/Users/zj-db0731/Downloads/testVideos/test2.mp4";


void openCodecContext(AVFormatContext * formatContext,AVCodecContext ** codecContext,int * streamIndex,int mediaType) {
    *streamIndex = av_find_best_stream(formatContext, mediaType, -1, -1, NULL, 0);
    if(*streamIndex < 0) {
        AVStream * stream = formatContext->streams[*streamIndex];
        AVCodec * codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if(!codec) {
            
        }
        *codecContext = avcodec_alloc_context3(codec);  //avcodec_alloc_context3(NULL) + avcodec_open2(codecContext, codec, NULL)
        if(!codecContext) {
            
        }
        
        if(avcodec_parameters_to_context(*codecContext, stream->codecpar) < 0) {
            
        }
    }
    
}

int main()
{
    av_register_all();
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }
    

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    
    //av_dump_format(fmt_ctx, 0, src_filename, 0);
    //printf("\n");
    
    printf("filename:%s\n",fmt_ctx->filename);
    printf("nb_streams:%u\n",fmt_ctx->nb_streams);
    printf("bitrate:%lld\n",fmt_ctx->bit_rate);
    printf("start_time:%lld\n",fmt_ctx->start_time);
    printf("duration:%lld\n",fmt_ctx->duration);
    printf("packet_size:%u\n",fmt_ctx->packet_size);
    printf("max_delay:%d\n",fmt_ctx->max_delay);
    printf("name:%s\n",fmt_ctx->iformat->name);
    printf("long_name:%s\n",fmt_ctx->iformat->long_name);
    printf("extension:%s\n",fmt_ctx->iformat->extensions);
    printf("\n");
    

    
    AVStream *st;
    for(int i=0;i<fmt_ctx->nb_streams;++i) {
        st = fmt_ctx->streams[i];
        printf("index:%d\n",st->index);
        printf("id:%d\n",st->id);
        printf("time_base:%d/%d\n",st->time_base.num,st->time_base.den);
        printf("start_time:%lld\n",st->start_time);
        printf("duration:%lld\n",st->duration);
        printf("nb_frames:%lld\n",st->nb_frames);
        printf("bit_rate:%lld\n",st->codecpar->bit_rate);
        printf("codec_type:%s\n",av_get_media_type_string(st->codecpar->codec_type));
        printf("\n");
    }
    
    openCodecContext(fmt_ctx, &video_dec_ctx, &videoStreamIndex, AVMEDIA_TYPE_VIDEO);
    
    
    openCodecContext(fmt_ctx, &audio_dec_ctx, &audioStreamIndex, AVMEDIA_TYPE_AUDIO);
    printf("sample_rate:%d",audio_dec_ctx->sample_rate);
    printf("channels:%d",audio_dec_ctx->channels);
    printf("sample_fmt:%s",av_get_sample_fmt_name(audio_dec_ctx->sample_fmt));
    printf("channel_layout:%s",av_get_channel_name (audio_dec_ctx->channel_layout));
    AV_CH_FRONT_LEFT
}
