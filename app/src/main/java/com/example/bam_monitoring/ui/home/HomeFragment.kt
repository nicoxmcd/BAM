package com.example.bam_monitoring.ui.home

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.example.bam_monitoring.databinding.FragmentHomeBinding

class HomeFragment : Fragment() {

    // View Binding instance
    private var _binding: FragmentHomeBinding? = null
    private val binding get() = _binding!!

    // SharedPreferences file name and key
    private val PREFS_NAME = "user_profile"
    private val KEY_NAME = "name"

    // Flag to track logo transformation state
    private var isTransformed = false

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        // Inflate the layout using View Binding
        _binding = FragmentHomeBinding.inflate(inflater, container, false)
        return binding.root
    }

    // This method is called after onCreateView
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Access SharedPreferences
        val sharedPreferences = requireContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

        // Retrieve the stored name, default to null if not found
        val name = sharedPreferences.getString(KEY_NAME, null)

        // Update the greeting TextView based on whether a name is available
        binding.textHello.text = if (!name.isNullOrEmpty()) "Hello, $name!" else "Hello!"

        // Set click listener on the logo image to trigger animations
        binding.logoImage.setOnClickListener {
            if (!isTransformed) {
                // Animate the logo: shrink and move upward
                binding.logoImage.animate()
                    .scaleX(0.4f)
                    .scaleY(0.4f)
                    // Moves the logo upward by its current top offset plus an extra 20 pixels (adjust as needed)
                    .translationY(-binding.logoImage.top.toFloat() - 20)
                    .setDuration(500)
                    .start()

                // Animate the greeting text to fade out quickly
                binding.textHello.animate()
                    .alpha(0f)
                    .setDuration(300)
                    .start()

                isTransformed = true
            } else {
                // Reverse the animation: restore original size and position
                binding.logoImage.animate()
                    .scaleX(1f)
                    .scaleY(1f)
                    .translationY(0f)
                    .setDuration(500)
                    .start()

                // Fade the greeting text back in
                binding.textHello.animate()
                    .alpha(1f)
                    .setDuration(300)
                    .start()

                isTransformed = false
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        // Nullify the binding reference to avoid memory leaks
        _binding = null
    }
}

