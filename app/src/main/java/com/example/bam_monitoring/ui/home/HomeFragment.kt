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
        inflater: LayoutInflater,
        container: ViewGroup?,
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

        val settingsPrefs = requireContext().getSharedPreferences(SETTINGS_PREFS_NAME, Context.MODE_PRIVATE)
        val isDebugMode = settingsPrefs.getBoolean(DEBUG_MODE_KEY, false)
        if (isDebugMode) {
            binding.textDiagnostic.visibility = View.VISIBLE
            binding.progressBarRelaxed.visibility = View.GONE
            binding.progressBarFlexed.visibility = View.GONE
            binding.progressBarStrained.visibility = View.GONE
        } else {
            binding.textDiagnostic.visibility = View.GONE
            // Ensure progress bars are initially hidden.
            binding.progressBarRelaxed.visibility = View.GONE
            binding.progressBarFlexed.visibility = View.GONE
            binding.progressBarStrained.visibility = View.GONE
        }

        var isScanning = false

        binding.logoImage.setOnClickListener {
            if (!isScanning) {
                isScanning = true
                // Animate logo and start scanning.
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
                if (!isDebugMode) {
                    // Reset, make visible and animate each progress bar.
                    listOf(binding.progressBarRelaxed, binding.progressBarFlexed, binding.progressBarStrained).forEach { bar ->
                        bar.progress = 0
                        bar.alpha = 0f
                        bar.visibility = View.VISIBLE
                        bar.animate().alpha(1f).setDuration(300).start()
                    }
                } else {
                    binding.textDiagnostic.text = ""
                    binding.textDiagnostic.animate().alpha(1f).setDuration(300).start()
                }
            } else {
                isScanning = false
                // Animate logo back and stop scanning.
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
                if (!isDebugMode) {
                    // Fade out each progress bar and set to GONE
                    listOf(binding.progressBarRelaxed, binding.progressBarFlexed, binding.progressBarStrained).forEach { bar ->
                        bar.animate().alpha(0f).setDuration(300)
                            .withEndAction { bar.visibility = View.GONE }
                            .start()
                    }
                } else {
                    binding.textDiagnostic.animate().alpha(0f).setDuration(300).start()
                }
            }
        }

        (activity as? MainActivity)?.bleManager?.diagnosticListener =
            object : BleManager.DiagnosticListener {
                override fun onDiagnostic(message: String) {
                    requireActivity().runOnUiThread {
                        val settingsPrefs = requireContext().getSharedPreferences(SETTINGS_PREFS_NAME, Context.MODE_PRIVATE)
                        val isDebugMode = settingsPrefs.getBoolean(DEBUG_MODE_KEY, false)


                        if (isDebugMode) {
                            if (diagnosticLines.size >= 8) diagnosticLines.removeFirst()
                            diagnosticLines.addLast(message)
                            binding.textDiagnostic.text = diagnosticLines.joinToString(separator = "\n")
                        } else {

                            // Expected message format: "Muscle strain data; F: XX%, R: YY%, S: ZZ%"
                            // Note that flexed and relaxed values are swapped in the message, so the flexed bar goes in the middle
                            val regex = Regex("Muscle\\s+strain\\s+data:\\s*R:\\s*(\\d+),\\s*F:\\s*(\\d+),\\s*S:\\s*(\\d+)", RegexOption.DOT_MATCHES_ALL)
                            regex.find(message)?.destructured?.let { (rVal, fVal, sVal) ->
                                binding.progressBarRelaxed.progress = rVal.toInt()
                                binding.progressBarFlexed.progress = fVal.toInt()
                                binding.progressBarStrained.progress = sVal.toInt()
                            } ?: run {
                                binding.progressBarFlexed.progress = 0
                                binding.progressBarRelaxed.progress = 0
                                binding.progressBarStrained.progress = 0
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