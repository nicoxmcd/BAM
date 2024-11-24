// com/example/bam_monitoring/ui/settings/SettingsFragment.kt
package com.example.bam_monitoring.ui.settings

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatDelegate
import androidx.fragment.app.Fragment
import com.example.bam_monitoring.databinding.FragmentSettingsBinding

class SettingsFragment : Fragment() {

    private var _binding: FragmentSettingsBinding? = null
    private val binding get() = _binding!!

    private val PREFS_NAME = "settings"
    private val DARK_MODE_KEY = "dark_mode"

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSettingsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Access SharedPreferences
        val sharedPreferences = requireContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val isDarkMode = sharedPreferences.getBoolean(DARK_MODE_KEY, false)

        // Set the switch state based on the saved preference
        binding.themeToggle.isChecked = isDarkMode

        // Listen for switch state changes
        binding.themeToggle.setOnCheckedChangeListener { _, isChecked ->
            // Save the new preference
            sharedPreferences.edit().putBoolean(DARK_MODE_KEY, isChecked).apply()

            // Apply the selected theme
            if (isChecked) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            }

            // Recreate activity to apply the theme immediately
            activity?.recreate()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
