package com.example.bam_monitoring.ui

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.recyclerview.widget.RecyclerView
import com.example.bam_monitoring.R
import com.example.bam_monitoring.recommendations.RecommendationsFragment

class VideoAdapter(
    private val context: Context,
    private val videoIds: List<String>,
    private val gridInfo: RecommendationsFragment.VideoGridInfo
) : RecyclerView.Adapter<VideoAdapter.VideoViewHolder>() {

    inner class VideoViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val videoWebView: WebView = itemView.findViewById(R.id.videoWebView)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VideoViewHolder {
        val view = LayoutInflater.from(context).inflate(R.layout.video_item, parent, false)
        return VideoViewHolder(view)
    }

    override fun onBindViewHolder(holder: VideoViewHolder, position: Int) {
        val params = holder.itemView.layoutParams
        params.height = gridInfo.videoFrameHeight
        params.width = gridInfo.videoFrameWidth
        holder.itemView.layoutParams = params

        val videoId = videoIds[position]
        holder.videoWebView.layoutParams.width = gridInfo.videoFrameWidth
        holder.videoWebView.layoutParams.height = gridInfo.videoFrameHeight

        val embedUrl = "https://www.youtube.com/embed/$videoId?autoplay=0"
        holder.videoWebView.settings.javaScriptEnabled = true
        holder.videoWebView.webViewClient = WebViewClient()
        holder.videoWebView.loadUrl(embedUrl)
    }

    override fun getItemCount(): Int = videoIds.size
}