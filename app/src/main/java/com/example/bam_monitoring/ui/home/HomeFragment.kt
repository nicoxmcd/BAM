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

    private val PREFS_NAME = "user_profile"
    private val KEY_NAME = "name"
    private var isTransformed = false

    // Use an ArrayDeque to hold up to 8 diagnostic lines.
    private val diagnosticLines = ArrayDeque<String>()

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        // Set initial alphas: greeting visible, diagnostics hidden.
        binding.textHello.alpha = 1f
        binding.textDiagnostic.alpha = 0f

        // Update greeting text based on SharedPreferences.
        val sharedPreferences = requireContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val name = sharedPreferences.getString(KEY_NAME, null)
        binding.textHello.text = if (!name.isNullOrEmpty()) "Hello, $name!" else "Hello!"

        // Toggle animation and BLE scan on logo click.
        binding.logoImage.setOnClickListener {
            if (!isTransformed) {
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
                binding.textDiagnostic.text = ""
                binding.textDiagnostic.animate().alpha(1f).setDuration(300).start()
                isTransformed = true
            } else {
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
                binding.textDiagnostic.animate().alpha(0f).setDuration(300).start()
                isTransformed = false
            }
        }

        // Set the diagnostic listener with a rolling log.
        (activity as? MainActivity)?.bleManager?.diagnosticListener = object : BleManager.DiagnosticListener {
            override fun onDiagnostic(message: String) {
                requireActivity().runOnUiThread {
                    if (diagnosticLines.size >= 8) {
                        diagnosticLines.removeFirst()
                    }
                    diagnosticLines.addLast(message)
                    binding.textDiagnostic.text = diagnosticLines.joinToString(separator = "\n")
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
