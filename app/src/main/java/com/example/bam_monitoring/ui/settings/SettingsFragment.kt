package com.example.bam_monitoring.ui.settings

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.animation.AccelerateDecelerateInterpolator
import androidx.appcompat.app.AppCompatDelegate
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import com.example.bam_monitoring.R
import com.example.bam_monitoring.databinding.FragmentSettingsBinding

class SettingsFragment : Fragment() {

    private var _binding: FragmentSettingsBinding? = null
    private val binding get() = _binding!!

    private val PREFS_NAME = "settings"
    private val DARK_MODE_KEY = "dark_mode"
    private val APPEARANCE_EXPANDED_KEY = "appearance_expanded"

    private var isAppearanceExpanded = false

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSettingsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Restore isAppearanceExpanded state
        if (savedInstanceState != null) {
            isAppearanceExpanded = savedInstanceState.getBoolean(APPEARANCE_EXPANDED_KEY, false)
        }

        // Access SharedPreferences
        val sharedPreferences = requireContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val isDarkMode = sharedPreferences.getBoolean(DARK_MODE_KEY, false)

        // Initialize Dark Mode Switch
        binding.themeToggle.isChecked = isDarkMode

        // Set visibility of Dark Mode Switch based on isAppearanceExpanded
        if (isAppearanceExpanded) {
            binding.themeToggle.visibility = View.VISIBLE
        } else {
            binding.themeToggle.visibility = View.GONE
        }

        // Handle Appearance Option Click
        binding.optionAppearance.setOnClickListener {
            isAppearanceExpanded = !isAppearanceExpanded
            if (isAppearanceExpanded) {
                expand(binding.themeToggle)
            } else {
                collapse(binding.themeToggle)
            }
        }

        // Handle Dark Mode Switch Changes
        binding.themeToggle.setOnCheckedChangeListener { _, isChecked ->
            // Save the new preference
            sharedPreferences.edit().putBoolean(DARK_MODE_KEY, isChecked).apply()

            // Apply the selected theme
            val nightMode = if (isChecked) {
                AppCompatDelegate.MODE_NIGHT_YES
            } else {
                AppCompatDelegate.MODE_NIGHT_NO
            }
            AppCompatDelegate.setDefaultNightMode(nightMode)

            // Recreate activity to apply theme changes
            activity?.recreate()
        }

        // Navigate to ProfileFragment when "Profile" is clicked
        binding.optionProfile.setOnClickListener {
            findNavController().navigate(R.id.action_settings_to_profile)
        }

        // Handle other options (Add click listeners as needed)
        binding.optionNotification.setOnClickListener {
            // Implement action for Notification option
        }
        binding.optionExportData.setOnClickListener {
            // Implement action for Export Data option
        }
        binding.optionTest.setOnClickListener {
            // Implement action for Test option
        }
        binding.optionTutorial.setOnClickListener {
            // Implement action for Tutorial option
        }
        binding.optionHelp.setOnClickListener {
            // Implement action for Help option
        }
        binding.optionAboutUs.setOnClickListener {
            // Implement action for About Us option
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        // Save the isAppearanceExpanded state
        outState.putBoolean(APPEARANCE_EXPANDED_KEY, isAppearanceExpanded)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    // Utility functions for expand and collapse animations
    private fun expand(view: View) {
        // Measure the target height of the view
        view.measure(
            View.MeasureSpec.makeMeasureSpec((view.parent as View).width, View.MeasureSpec.EXACTLY),
            View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
        )
        val targetHeight = view.measuredHeight

        // Set initial height to 0 and make visible
        view.layoutParams.height = 0
        view.visibility = View.VISIBLE

        val anim = ValueAnimator.ofInt(0, targetHeight)
        anim.addUpdateListener { valueAnimator ->
            val layoutParams = view.layoutParams
            layoutParams.height = valueAnimator.animatedValue as Int
            view.layoutParams = layoutParams
        }
        anim.duration = 200
        anim.interpolator = AccelerateDecelerateInterpolator()
        anim.start()
    }

    private fun collapse(view: View) {
        val initialHeight = view.measuredHeight

        val anim = ValueAnimator.ofInt(initialHeight, 0)
        anim.addUpdateListener { valueAnimator ->
            val layoutParams = view.layoutParams
            layoutParams.height = valueAnimator.animatedValue as Int
            view.layoutParams = layoutParams
        }
        anim.duration = 200
        anim.interpolator = AccelerateDecelerateInterpolator()
        anim.addListener(object : AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                view.visibility = View.GONE
            }
        })
        anim.start()
    }
}
