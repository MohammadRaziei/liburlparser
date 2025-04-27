import React, { useState } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faPython, faCode, faCopy, faCheck } from '@fortawesome/free-solid-svg-icons';
import { faGithub } from '@fortawesome/free-brands-svg-icons';

type Language = 'python' | 'cpp';
type Method = 'pip' | 'git' | 'cmake' | 'apt';

const InstallationGuide: React.FC = () => {
  const [language, setLanguage] = useState<Language>('python');
  const [method, setMethod] = useState<Method>('pip');
  const [copied, setCopied] = useState(false);

  const languageOptions = [
    { value: 'python', label: 'Python', icon: <FontAwesomeIcon icon={faPython} /> },
    { value: 'cpp', label: 'C++', icon: <FontAwesomeIcon icon={faCode} /> },
  ];

  const methodOptions = {
    python: [
      { value: 'pip', label: 'Pip', icon: <FontAwesomeIcon icon={faPython} /> },
      { value: 'git', label: 'From Source (Git)', icon: <FontAwesomeIcon icon={faGithub} /> },
    ],
    cpp: [
      { value: 'cmake', label: 'CMake (Build from source)', icon: <FontAwesomeIcon icon={faCode} /> },
      { value: 'apt', label: 'Package Manager (apt)', icon: <FontAwesomeIcon icon={faCode} /> },
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
    },
    cpp: {
      cmake: `# Clone the repository
git clone https://github.com/mohammadraziei/liburlparser.git
cd liburlparser
mkdir -p build && cd build
cmake ..
make
sudo make install`,
      apt: `# Add the repository (coming soon)
sudo apt update
sudo apt install liburlparser-dev`,
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


  return (
    <div className="section-container">
      <h2 className="section-title">
        <FontAwesomeIcon icon={faCode} className="text-[var(--primary-blue)] mr-3" />
        Installation Guide
      </h2>
      
      <div className="bg-white rounded-lg shadow-md overflow-hidden mb-4 border border-gray-200">
        <table className="w-full">
          <tbody>
            {/* Row 1: Language Selection */}
            <tr className="border-b border-gray-200">
              <td className="bg-gray-50 font-medium p-1.5 w-1/4 border-r border-gray-200">
                <div className="text-[var(--primary-dark)]">Step 1: Select Language</div>
              </td>
              <td className="p-1.5">
                <div className="flex space-x-2">
                  {languageOptions.map((option) => (
                    <button
                      key={option.value}
                      className={`px-2 py-1 rounded-md flex items-center space-x-1.5 transition-all ${
                        language === option.value
                          ? 'bg-gradient-to-r from-[var(--primary-blue)]/15 to-[var(--primary-yellow)]/15 text-[var(--primary-dark)] font-medium'
                          : 'bg-gray-100 hover:bg-gray-200 text-gray-700'
                      }`}
                      onClick={() => {
                        setLanguage(option.value as Language);
                        setMethod(methodOptions[option.value as Language][0].value as Method);
                      }}
                    >
                      <div className={`text-base ${language === option.value ? 'text-[var(--primary-dark)]' : 'text-[var(--primary-blue)]'}`}>
                        {option.icon}
                      </div>
                      <span>{option.label}</span>
                    </button>
                  ))}
                </div>
              </td>
            </tr>
            
            {/* Row 2: Installation Method */}
            <tr>
              <td className="bg-gray-50 font-medium p-1.5 w-1/4 border-r border-gray-200">
                <div className="text-[var(--primary-dark)]">Step 2: Select Installation Method</div>
              </td>
              <td className="p-1.5">
                <div className="flex flex-wrap gap-2">
                  {methodOptions[language].map((option) => (
                    <button
                      key={option.value}
                      className={`px-2 py-1 rounded-md flex items-center space-x-1.5 transition-all ${
                        method === option.value
                          ? 'bg-gradient-to-r from-[var(--primary-blue)]/15 to-[var(--primary-yellow)]/15 text-[var(--primary-dark)] font-medium'
                          : 'bg-gray-100 hover:bg-gray-200 text-gray-700'
                      }`}
                      onClick={() => setMethod(option.value as Method)}
                    >
                      <div className={`text-base ${method === option.value ? 'text-[var(--primary-dark)]' : 'text-[var(--primary-blue)]'}`}>
                        {option.icon}
                      </div>
                      <span>{option.label}</span>
                    </button>
                  ))}
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
      
      <div className="space-y-4">
        <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
          <div className="flex justify-between items-center bg-gray-50 text-[var(--primary-dark)] px-2 py-1">
            <h3 className="font-medium">Installation Commands</h3>
            <button 
              onClick={() => copyToClipboard(installationCommands[language][method])}
              className="flex items-center space-x-1 text-xs bg-white hover:bg-gray-100 rounded px-1.5 py-0.5 transition-colors text-[var(--primary-dark)] border border-gray-200"
            >
              <FontAwesomeIcon icon={copied ? faCheck : faCopy} />
              <span>{copied ? "Copied!" : "Copy"}</span>
            </button>
          </div>
          <pre className="bg-[#1e1e1e] text-white p-2 m-0 overflow-x-auto">
            <code>{installationCommands[language][method]}</code>
          </pre>
        </div>
        
        <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
          <div className="flex justify-between items-center bg-gray-50 text-[var(--primary-dark)] px-2 py-1">
            <h3 className="font-medium">Basic Usage Example</h3>
            <button 
              onClick={() => copyToClipboard(usageExamples[language])}
              className="flex items-center space-x-1 text-xs bg-white hover:bg-gray-100 rounded px-1.5 py-0.5 transition-colors text-[var(--primary-dark)] border border-gray-200"
            >
              <FontAwesomeIcon icon={copied ? faCheck : faCopy} />
              <span>{copied ? "Copied!" : "Copy"}</span>
            </button>
          </div>
          <pre className="bg-[#1e1e1e] text-white p-2 m-0 overflow-x-auto">
            <code>{usageExamples[language]}</code>
          </pre>
        </div>
      </div>
    </div>
  );
};

export default InstallationGuide;