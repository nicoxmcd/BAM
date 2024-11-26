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
        if (!name.isNullOrEmpty()) {
            binding.textHello.text = "Hello, $name!"
        } else {
            binding.textHello.text = "Hello!"
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        // Nullify the binding reference to avoid memory leaks
        _binding = null
    }
}
