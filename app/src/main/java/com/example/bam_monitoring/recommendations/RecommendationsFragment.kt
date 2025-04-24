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

    // Search term & API key
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

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? = inflater.inflate(R.layout.fragment_recommendations, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        recyclerView = view.findViewById(R.id.recyclerView)
        val gridInfo = calculateVideoGridInfo()
        recyclerView.layoutManager = GridLayoutManager(requireContext(), gridInfo.numColumns)
        searchVideos(queryString, gridInfo)
    }

    private fun searchVideos(query: String, gridInfo: VideoGridInfo) {
        Thread {
            try {
                // Initial search, only embeddable & syndicated
                val encodedQuery = URLEncoder.encode(query, "UTF-8")
                val maxResults = gridInfo.numColumns * gridInfo.numRows
                val searchUrl = "https://www.googleapis.com/youtube/v3/search" +
                        "?part=snippet" +
                        "&maxResults=$maxResults" +
                        "&type=video" +
                        "&q=$encodedQuery" +
                        "&videoEmbeddable=true" +
                        "&videoSyndicated=true" +
                        "&key=$apiKey"

                val searchConn = URL(searchUrl).openConnection() as HttpURLConnection
                searchConn.requestMethod = "GET"
                val searchResp = searchConn.inputStream.bufferedReader().use { it.readText() }
                val rawIds = parseVideoIds(searchResp)

                // Filter out any that are private / unprocessed / non-embeddable / region-restricted
                val availableIds = filterAvailableVideos(rawIds)

                activity?.runOnUiThread {
                    if (!isAdded) return@runOnUiThread

                    if (availableIds.isNotEmpty()) {
                        videoAdapter = VideoAdapter(requireContext(), availableIds, gridInfo)
                        recyclerView.adapter = videoAdapter
                    } else {
                        Toast.makeText(requireContext(), "No videos found.", Toast.LENGTH_LONG).show()
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
                activity?.runOnUiThread {
                    if (isAdded) {
                        Toast.makeText(requireContext(), "Error: ${e.message}", Toast.LENGTH_LONG).show()
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

    private fun filterAvailableVideos(videoIds: List<String>): List<String> {
        val goodOnes = mutableListOf<String>()
        if (videoIds.isEmpty()) return goodOnes

        // YouTube videos.list only accepts up to 50 IDs per call
        videoIds.chunked(50).forEach { chunk ->
            val idParam = chunk.joinToString(",")
            val statusUrl = "https://www.googleapis.com/youtube/v3/videos" +
                    "?part=status,contentDetails" +
                    "&id=$idParam" +
                    "&key=$apiKey"

            val conn = URL(statusUrl).openConnection() as HttpURLConnection
            conn.requestMethod = "GET"
            val resp = conn.inputStream.bufferedReader().use { it.readText() }
            val json = JSONObject(resp)
            val items = json.getJSONArray("items")

            for (i in 0 until items.length()) {
                val item = items.getJSONObject(i)
                val status = item.getJSONObject("status")

                // 1) must be embeddable
                if (!status.optBoolean("embeddable", false)) continue

                // 2) must be public & fully processed
                if (status.getString("privacyStatus") != "public"
                    || status.getString("uploadStatus") != "processed"
                ) continue

                // 3) drop anything with region restrictions
                val cd = item.getJSONObject("contentDetails")
                if (cd.has("regionRestriction")) {
                    val rr = cd.getJSONObject("regionRestriction")
                    if (rr.has("blocked")) continue
                    if (rr.has("allowed")) continue
                }

                goodOnes += item.getString("id")
            }
        }

        return goodOnes
    }
}
