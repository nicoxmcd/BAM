package com.example.bam_monitoring.ui.profile

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import com.example.bam_monitoring.databinding.FragmentProfileBinding

class ProfileFragment : Fragment() {

    private var _binding: FragmentProfileBinding? = null
    private val binding get() = _binding!!

    private val PREFS_NAME = "user_profile"

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentProfileBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        // Hide the Up button
        (activity as AppCompatActivity).supportActionBar?.setDisplayHomeAsUpEnabled(false)

        // Load saved data
        val sharedPreferences = requireContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        binding.editTextName.setText(sharedPreferences.getString("name", ""))
        binding.editTextHeight.setText(sharedPreferences.getString("height", ""))
        binding.editTextWeight.setText(sharedPreferences.getString("weight", ""))
        binding.editTextSex.setText(sharedPreferences.getString("sex", ""))

        // Save data when Save button is clicked
        binding.buttonSave.setOnClickListener {
            val editor = sharedPreferences.edit()
            editor.putString("name", binding.editTextName.text.toString())
            editor.putString("height", binding.editTextHeight.text.toString())
            editor.putString("weight", binding.editTextWeight.text.toString())
            editor.putString("sex", binding.editTextSex.text.toString())
            editor.apply()
            Toast.makeText(requireContext(), "Profile saved", Toast.LENGTH_SHORT).show()
        }

        // Handle back button press
        binding.buttonBack.setOnClickListener {
            findNavController().navigateUp()
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
