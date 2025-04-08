package com.example.bam_monitoring.recommendations

import android.os.Bundle
import android.util.DisplayMetrics
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.bam_monitoring.R
import com.example.bam_monitoring.ui.VideoAdapter
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

class RecommendationsFragment : Fragment() {

    private lateinit var recyclerView: RecyclerView
    private lateinit var videoAdapter: VideoAdapter
    private val queryString = "helpful exercises"
    private val apiKey = "AIzaSyA_Y8I8yj-2sYnwYrxaz-u1depV5bqlv7c"

    data class VideoGridInfo(
        val numColumns: Int,
        val numRows: Int,
        val videoFrameWidth: Int,
        val videoFrameHeight: Int
    )

    private fun calculateVideoGridInfo(): VideoGridInfo {
        val displayMetrics: DisplayMetrics = resources.displayMetrics
        val spacing = (16 * displayMetrics.density).toInt()
        val videoFrameHeight = 216 + spacing / 2
        val videoFrameWidth = ((videoFrameHeight * 16) / 9) + spacing / 2
        val numColumns = (displayMetrics.widthPixels / videoFrameWidth).coerceAtLeast(1)
        var numRows = ((displayMetrics.heightPixels - spacing) / (216 + spacing)).coerceAtLeast(1)
        if (numRows > 1 && (displayMetrics.heightPixels - spacing) % (216 + spacing) < videoFrameHeight) {
            numRows--
        }
        return VideoGridInfo(numColumns, numRows - 1, videoFrameWidth, videoFrameHeight)
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_recommendations, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        recyclerView = view.findViewById(R.id.recyclerView)
        val gridInfo = calculateVideoGridInfo()
        recyclerView.layoutManager = GridLayoutManager(requireContext(), gridInfo.numColumns)
        searchVideos(queryString, gridInfo)
    }

    private fun searchVideos(query: String, gridInfo: VideoGridInfo) {
        val encodedQuery = URLEncoder.encode(query, "UTF-8")
        val maxResults = gridInfo.numColumns * gridInfo.numRows
        val urlString = "https://www.googleapis.com/youtube/v3/search?part=snippet" +
                "&maxResults=$maxResults&type=video&q=$encodedQuery&key=$apiKey"

        Thread {
            try {
                val url = URL(urlString)
                val connection = url.openConnection() as HttpURLConnection
                connection.requestMethod = "GET"
                val response = connection.inputStream.bufferedReader().use { it.readText() }
                val videoIds = parseVideoIds(response)
                activity?.runOnUiThread {
                    // Only update UI if fragment is still added
                    if (isAdded) {
                        if (videoIds.isNotEmpty()) {
                            videoAdapter = VideoAdapter(requireContext(), videoIds, gridInfo)
                            recyclerView.adapter = videoAdapter
                        } else {
                            Toast.makeText(requireContext(), "No videos found.", Toast.LENGTH_LONG).show()
                        }
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
                activity?.runOnUiThread {
                    if (isAdded) {
                        Toast.makeText(requireContext(), "Error: " + e.message, Toast.LENGTH_LONG).show()
                    }
                }
            }
        }.start()
    }

    private fun parseVideoIds(response: String): List<String> {
        val videoIds = mutableListOf<String>()
        val json = JSONObject(response)
        val items = json.getJSONArray("items")
        for (i in 0 until items.length()) {
            val videoId = items.getJSONObject(i)
                .getJSONObject("id")
                .getString("videoId")
            videoIds.add(videoId)
        }
        return videoIds
    }
}