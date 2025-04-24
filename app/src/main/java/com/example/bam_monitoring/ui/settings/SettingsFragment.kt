package com.example.bam_monitoring.ui.settings

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
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
    private val DEBUG_MODE_KEY = "debug_mode"
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

        if (savedInstanceState != null) {
            isAppearanceExpanded = savedInstanceState.getBoolean(APPEARANCE_EXPANDED_KEY, false)
        }

        val sharedPreferences = requireContext().getSharedPreferences(PREFS_NAME, android.content.Context.MODE_PRIVATE)
        val isDarkMode = sharedPreferences.getBoolean(DARK_MODE_KEY, false)
        val isDebugMode = sharedPreferences.getBoolean(DEBUG_MODE_KEY, false)

        binding.themeToggle.isChecked = isDarkMode
        binding.debugToggle.isChecked = isDebugMode

        if (isAppearanceExpanded) {
            binding.themeToggle.visibility = View.VISIBLE
            binding.debugToggle.visibility = View.VISIBLE
        } else {
            binding.themeToggle.visibility = View.GONE
            binding.debugToggle.visibility = View.GONE
        }

        binding.optionAppearance.setOnClickListener {
            isAppearanceExpanded = !isAppearanceExpanded
            if (isAppearanceExpanded) {
                expand(binding.themeToggle)
                expand(binding.debugToggle)
            } else {
                collapse(binding.themeToggle)
                collapse(binding.debugToggle)
            }
        }

        binding.themeToggle.setOnCheckedChangeListener { _, isChecked ->
            sharedPreferences.edit().putBoolean(DARK_MODE_KEY, isChecked).apply()
            val nightMode = if (isChecked) AppCompatDelegate.MODE_NIGHT_YES else AppCompatDelegate.MODE_NIGHT_NO
            AppCompatDelegate.setDefaultNightMode(nightMode)
            activity?.recreate()
        }

        binding.debugToggle.setOnCheckedChangeListener { _, isChecked ->
            sharedPreferences.edit().putBoolean(DEBUG_MODE_KEY, isChecked).apply()
        }

        binding.optionProfile.setOnClickListener {
            findNavController().navigate(R.id.action_settings_to_profile)
        }
        binding.optionNotification.setOnClickListener { }
        binding.optionExportData.setOnClickListener { }
        binding.optionTest.setOnClickListener { }
        binding.optionTutorial.setOnClickListener { }
        binding.optionHelp.setOnClickListener { }
        binding.optionAboutUs.setOnClickListener {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://bam-monitoring.my.canva.site/#our-team"))
            startActivity(intent)
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBoolean(APPEARANCE_EXPANDED_KEY, isAppearanceExpanded)
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }

    private fun expand(view: View) {
        view.measure(
            View.MeasureSpec.makeMeasureSpec((view.parent as View).width, View.MeasureSpec.EXACTLY),
            View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED)
        )
        val targetHeight = view.measuredHeight
        view.layoutParams.height = 0
        view.visibility = View.VISIBLE

        val anim = android.animation.ValueAnimator.ofInt(0, targetHeight)
        anim.addUpdateListener { valueAnimator ->
            val layoutParams = view.layoutParams
            layoutParams.height = valueAnimator.animatedValue as Int
            view.layoutParams = layoutParams
        }
        anim.duration = 200
        anim.interpolator = android.view.animation.AccelerateDecelerateInterpolator()
        anim.start()
    }

    private fun collapse(view: View) {
        val initialHeight = view.measuredHeight
        val anim = android.animation.ValueAnimator.ofInt(initialHeight, 0)
        anim.addUpdateListener { valueAnimator ->
            val layoutParams = view.layoutParams
            layoutParams.height = valueAnimator.animatedValue as Int
            view.layoutParams = layoutParams
        }
        anim.duration = 200
        anim.interpolator = android.view.animation.AccelerateDecelerateInterpolator()
        anim.addListener(object : android.animation.AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: android.animation.Animator) {
                view.visibility = View.GONE
            }
        })
        anim.start()
    }
}