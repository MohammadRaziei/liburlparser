import React, { useState } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
// Add faTerminal
import { faCode, faCopy, faCheck, faTag, faTerminal } from '@fortawesome/free-solid-svg-icons'; 
import { faGithub, faPython } from '@fortawesome/free-brands-svg-icons';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/cjs/styles/prism';
import Container from './Container'; // Import the new Container


// Add 'bash' to Language type
type Language = 'python' | 'cpp' | 'bash'; 
// Rename 'git_http' to 'pip_git' in Method type
type Method = 'pip' | 'git' | 'cmake' | 'sub_directory' | 'pip_git'; 
type Version = 'latest' | '1.0.0'; // Example versions, adjust as needed

const InstallationGuide: React.FC = () => {
  const [language, setLanguage] = useState<Language>('python');
  // Default method remains 'pip'
  const [method, setMethod] = useState<Method>('pip'); 
  const [version, setVersion] = useState<Version>('latest'); 
  const [copied, setCopied] = useState(false);

  const languageOptions = [
    { value: 'python', label: 'Python', icon: <FontAwesomeIcon icon={faPython} /> },
    { value: 'cpp', label: 'C++', icon: <FontAwesomeIcon icon={faCode} /> },
    // Add Bash option
    { value: 'bash', label: 'Bash CLI', icon: <FontAwesomeIcon icon={faTerminal} /> }, 
  ];

  const methodOptions = {
    python: [
      { value: 'pip', label: 'Pip', icon: <FontAwesomeIcon icon={faPython} /> },
      { value: 'pip_git', label: 'From Source (Pip + Git)', icon: <FontAwesomeIcon icon={faGithub} /> }, 
      { value: 'git', label: 'From Source (Git Clone)', icon: <FontAwesomeIcon icon={faGithub} /> },
    ],
    cpp: [
      { value: 'cmake', label: 'CMake (Build from source)', icon: <FontAwesomeIcon icon={faCode} /> },
      { value: 'sub_directory', label: 'CMake (Add as Subdirectory)', icon: <FontAwesomeIcon icon={faCode} /> },
    ],
    // Define methods for Bash - only pip is relevant for installing the CLI tool
    bash: [
      { value: 'pip', label: 'Pip (Installs CLI)', icon: <FontAwesomeIcon icon={faPython} /> }, 
    ],
  };

  const installationCommands = {
    python: {
      pip: `# Install from PyPI
pip install liburlparser`,
      git: `# Clone the repository
git clone https://github.com/mohammadraziei/liburlparser.git
cd liburlparser
pip install .`,
      // Add command for Git+HTTP
      pip_git: `# Install directly from GitHub repository URL using pip
pip install git+https://github.com/mohammadraziei/liburlparser.git`, 
    },
    cpp: {
      cmake: `# Clone the repository
git clone https://github.com/mohammadraziei/liburlparser.git
cd liburlparser
mkdir -p build && cd build
cmake ..
make
sudo make install`,
      // Replace 'apt' command with 'sub_directory' instructions
      sub_directory: `# 1. Add liburlparser as a submodule or copy it into your project
# Example using git submodule:
git submodule add https://github.com/mohammadraziei/liburlparser.git extern/liburlparser

# 2. In your main CMakeLists.txt:
# Add the subdirectory
add_subdirectory(extern/liburlparser)

# Link against the library
target_link_libraries(your_target_name PRIVATE urlparser)`,
    },
    // Add commands for Bash (which is just installing the Python package)
    bash: {
      pip: `# Install the Python package which provides the CLI tool
pip install liburlparser`,
    },
  };

  const usageExamples = {
    python: `from liburlparser import Url, Host

# Parse URL
url = Url("https://ee.aut.ac.ir/#id")
print(url.suffix, url.domain, url.fragment)

# Parse host
host = Host("ee.aut.ac.ir")
print(host.domain, host.suffix)`,
    cpp: `#include <iostream>
#include <liburlparser/url.h>

int main() {
    // Parse URL
    liburlparser::Url url("https://ee.aut.ac.ir/#id");
    std::cout << url.suffix() << " " << url.domain() << " " << url.fragment() << std::endl;
    
    // Parse host
    liburlparser::Host host("ee.aut.ac.ir");
    std::cout << host.domain() << " " << host.suffix() << std::endl;
    
    return 0;
}`,
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };


  // Helper function to get the correct command safely
  const getCommand = (lang: Language, meth: Method, ver: Version): string => {
    // Update Python condition to include 'pip_git' instead of 'git_http'
    if (lang === 'python' && (meth === 'pip' || meth === 'git' || meth === 'pip_git')) { 
      // Ensure the key matches the renamed key
      return installationCommands.python[meth as 'pip' | 'git' | 'pip_git']; 
    }
    if (lang === 'cpp' && (meth === 'cmake' || meth === 'sub_directory')) {
      return installationCommands.cpp[meth];
    }
    if (lang === 'bash' && meth === 'pip') { 
      return installationCommands.bash.pip;
    }
    // Fallback
    console.error("Invalid language/method combination:", lang, meth);
    return "Error: Invalid selection";
  };

  // Update currentCommand to handle potential undefined methods if logic changes
  const currentCommand = getCommand(language, method, version);

  // Function to handle language change and reset method appropriately
  const handleLanguageChange = (newLang: Language) => {
    setLanguage(newLang);
    // Reset method to the first available for the new language
    const firstMethod = methodOptions[newLang][0].value as Method;
    setMethod(firstMethod);
  };


  return (
    <Container className="py-12"> 
      <div> 
        <h2 className="section-title">
          <FontAwesomeIcon icon={faCode} className="text-[#3871a2] mr-3" />
          Installation Guide
        </h2>
        
        <div className="bg-white rounded-lg shadow-md overflow-hidden mb-4 border border-gray-200">
          <table className="w-full">
            <tbody>
              {/* Row 1: Language Selection */}
              <tr className="border-b border-gray-200">
                <td className="bg-gray-50 font-medium p-1.5 w-1/4 border-r border-gray-200">
                  <div className="text-[#231f20]">Step 1: Select Language</div>
                </td>
                <td className="p-1.5">
                  <div className="flex space-x-2">
                    {languageOptions.map((option) => (
                      <button
                        key={option.value}
                        // Update onClick to use the new handler
                        onClick={() => handleLanguageChange(option.value as Language)} 
                        className={`px-2 py-1 rounded-md flex items-center space-x-1.5 transition-all ${
                          language === option.value
                            ? 'btn-selected-gradient' 
                            : 'bg-gray-100 hover:bg-gray-200 text-gray-700'
                        }`}
                      >
                        {/* Replace var() with hex codes */}
                        <div className={`text-base ${language === option.value ? 'text-[#3871a2]' : 'text-[#231f20]'}`}>
                          {option.icon}
                        </div>
                        <span>{option.label}</span>
                      </button>
                    ))}
                  </div>
                </td>
              </tr>
              
              {/* Row 2: Installation Method - Conditionally render or adjust based on language */}
              {/* Only show method selection if not Bash, or adjust options */}
              {language !== 'bash' && ( // Simple approach: hide for Bash
                <tr className="border-b border-gray-200">
                  <td className="bg-gray-50 font-medium p-1.5 w-1/4 border-r border-gray-200">
                    {/* Replace var() with hex codes */}
                    <div className="text-[#231f20]">Step 2: Select Installation Method</div>
                  </td>
                  <td className="p-1.5">
                    <div className="flex flex-wrap gap-2">
                      {/* Ensure methodOptions[language] exists before mapping */}
                      {methodOptions[language] && methodOptions[language].map((option) => ( 
                        <button
                          key={option.value}
                          className={`px-2 py-1 rounded-md flex items-center space-x-1.5 transition-all ${
                            method === option.value
                              ? 'btn-selected-gradient' 
                              : 'bg-gray-100 hover:bg-gray-200 text-gray-700'
                          }`}
                          onClick={() => setMethod(option.value as Method)}
                        >
                          {/* Replace var() with hex codes */}
                          <div className={`text-base ${method === option.value ? 'text-[#3871a2]' : 'text-[#231f20]'}`}>
                            {option.icon}
                          </div>
                          <span>{option.label}</span>
                        </button>
                      ))}
                    </div>
                  </td>
                </tr>
              )}
              {/* You could add a row here specifically for Bash if needed, */}
              {/* e.g., to confirm 'pip' is the method */}

            </tbody>
          </table>
        </div>
        
        <div className="space-y-4">
          <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
            {/* Replace var() with hex codes */}
            <div className="flex justify-between items-center bg-gray-50 text-[#231f20] px-2 py-1">
              <h3 className="font-medium">Installation Commands</h3>
              <button 
                onClick={() => copyToClipboard(currentCommand)} 
                // Replace var() with hex codes
                className="flex items-center space-x-1 text-xs bg-white hover:bg-gray-100 rounded px-1.5 py-0.5 transition-colors text-[#231f20] border border-gray-200"
              >
                <FontAwesomeIcon icon={copied ? faCheck : faCopy} />
                <span>{copied ? "Copied!" : "Copy"}</span>
              </button>
            </div>
            <SyntaxHighlighter 
              language="bash" 
              style={vscDarkPlus} 
              customStyle={{ margin: 0, padding: '0.5rem', borderRadius: '0 0 0.375rem 0.375rem' }} 
              wrapLongLines={true}
            >
              {currentCommand}
            </SyntaxHighlighter>
          </div>
        </div>
      </div>
    </Container>
  );
};

export default InstallationGuide;