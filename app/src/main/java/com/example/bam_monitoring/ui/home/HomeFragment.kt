package com.example.bam_monitoring.ui.home

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.example.bam_monitoring.MainActivity
import com.example.bam_monitoring.databinding.FragmentHomeBinding
import com.example.bam_monitoring.ble.BleManager
import java.util.ArrayDeque

class HomeFragment : Fragment() {

    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!

    private val PROFILE_PREFS_NAME = "user_profile"
    private val KEY_NAME = "name"
    private val SETTINGS_PREFS_NAME = "settings"
    private val DEBUG_MODE_KEY = "debug_mode"

    private val diagnosticLines = ArrayDeque<String>()

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        binding.textHello.alpha = 1f
        binding.textDiagnostic.alpha = 0f

        // Update greeting text.
        val profilePrefs = requireContext().getSharedPreferences(PROFILE_PREFS_NAME, Context.MODE_PRIVATE)
        val name = profilePrefs.getString(KEY_NAME, null)
        binding.textHello.text = if (!name.isNullOrEmpty()) "Hello, $name!" else "Hello!"

        // Retrieve debug mode setting.
        val settingsPrefs = requireContext().getSharedPreferences(SETTINGS_PREFS_NAME, Context.MODE_PRIVATE)
        val isDebugMode = settingsPrefs.getBoolean(DEBUG_MODE_KEY, false)
        if (isDebugMode) {
            binding.textDiagnostic.visibility = View.VISIBLE
            binding.progressBar.visibility = View.GONE
        } else {
            binding.textDiagnostic.visibility = View.GONE
            binding.progressBar.visibility = View.GONE // Initially hidden
        }

        // Use a flag to track scanning state.
        var isScanning = false

        binding.logoImage.setOnClickListener {
            if (!isScanning) {
                isScanning = true
                binding.logoImage.animate()
                    .scaleX(0.4f)
                    .scaleY(0.4f)
                    .translationY(-binding.logoImage.top.toFloat() - 20)
                    .setDuration(500)
                    .withEndAction {
                        (activity as? MainActivity)?.bleManager?.startScan()
                    }
                    .start()
                binding.textHello.animate().alpha(0f).setDuration(300).start()
                if (isDebugMode) {
                    binding.textDiagnostic.text = ""
                    binding.textDiagnostic.animate().alpha(1f).setDuration(300).start()
                } else {
                    binding.progressBar.alpha = 0f
                    binding.progressBar.visibility = View.VISIBLE // Show progress bar
                    binding.progressBar.animate().alpha(1f).setDuration(300).start()
                    binding.progressBar.progress = 0
                }
            } else {
                isScanning = false
                binding.logoImage.animate()
                    .scaleX(1f)
                    .scaleY(1f)
                    .translationY(0f)
                    .setDuration(500)
                    .withEndAction {
                        (activity as? MainActivity)?.bleManager?.stopScan()
                    }
                    .start()
                binding.textHello.animate().alpha(1f).setDuration(300).start()
                if (isDebugMode) {
                    binding.textDiagnostic.animate().alpha(0f).setDuration(300).start()
                } else {
                    binding.progressBar.animate().alpha(0f).setDuration(300).withEndAction {
                        binding.progressBar.visibility = View.GONE // Hide progress bar after fade
                    }.start()
                }
            }
        }

        // Set the diagnostic listener.
        (activity as? MainActivity)?.bleManager?.diagnosticListener =
            object : BleManager.DiagnosticListener {
                override fun onDiagnostic(message: String) {
                    requireActivity().runOnUiThread {
                        val settingsPrefs = requireContext().getSharedPreferences(SETTINGS_PREFS_NAME, Context.MODE_PRIVATE)
                        val isDebugMode = settingsPrefs.getBoolean(DEBUG_MODE_KEY, false)
                        if (isDebugMode) {
                            if (diagnosticLines.size >= 8) {
                                diagnosticLines.removeFirst()
                            }
                            diagnosticLines.addLast(message)
                            binding.textDiagnostic.text = diagnosticLines.joinToString(separator = "\n")
                        } else {
                            message.toIntOrNull()?.let { percentage ->
                                binding.progressBar.progress = percentage.coerceIn(0, 100)
                            }
                        }
                    }
                }
            }
    }

    override fun onDestroyView() {
        (activity as? MainActivity)?.bleManager?.diagnosticListener = null
        _binding = null
        super.onDestroyView()
    }
}