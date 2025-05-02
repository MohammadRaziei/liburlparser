import React, { useState } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
// Add faTerminal
import { faCode, faCopy, faCheck, faLightbulb, faTerminal } from '@fortawesome/free-solid-svg-icons'; 
import { faPython } from '@fortawesome/free-brands-svg-icons';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/cjs/styles/prism';
import Container from './Container'; // Import the new Container

// Add 'bash' to Language type
type Language = 'python' | 'cpp' | 'bash'; 

const UsageExampleGuide: React.FC = () => {
  const [language, setLanguage] = useState<Language>('python');
  const [copied, setCopied] = useState(false);

  const languageOptions = [
    { value: 'python', label: 'Python', icon: <FontAwesomeIcon icon={faPython} /> },
    { value: 'cpp', label: 'C++', icon: <FontAwesomeIcon icon={faCode} /> },
    { value: 'bash', label: 'Bash CLI', icon: <FontAwesomeIcon icon={faTerminal} /> }, 
  ];

  const usageExamples = {
    python: `from liburlparser import Url, Host

# Parse URL
url = Url("https://ee.aut.ac.ir/#id")
print(url.suffix, url.domain, url.fragment)

# Parse host
host = Host("ee.aut.ac.ir")
print(host.domain, host.suffix)`,
    cpp: `#include "urlparser.h"
...
/// for parsing url
TLD::Url url("https://ee.aut.ac.ir/about");
std::string domain = url.domain(); // also for subdomain, port, params, ...
/// for parsing host
TLD::Host host("ee.aut.ac.ir");
// or
TLD::Host host = url.host();
// or
TLD::Host host = TLD::Host::fromUrl("https://ee.aut.ac.ir/about");`,
    // Add Bash CLI example
    bash: `# Parse a URL and print specific parts (e.g., domain, suffix)
python -m liburlparser --url "https://user:pass@www.example.co.uk:8080/path/to/file.html?query=1#fragment" --parts domain suffix

# Parse a host and print specific parts
python -m liburlparser --host "sub.example.co.uk" | jq

# Get all parts of a URL as JSON
python -m liburlparser --url "https://www.example.com/path?q=test" | jq`, 
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const currentUsageExample = usageExamples[language];

  return (
    // Use Container, add vertical padding (e.g., py-12 or py-16)
    <Container className="py-12"> 
        <h2 className="section-title">
          <FontAwesomeIcon icon={faLightbulb} className="text-[#ffd340] mr-3" />
          Basic Usage Example
        </h2>

        {/* Content remains the same */}
        {/* Language Selection Row */}
        <div className="bg-white rounded-lg shadow-md overflow-hidden mb-4 border border-gray-200">
         <table className="w-full">
           <tbody>
             <tr className="border-b border-gray-200">
               <td className="bg-gray-50 font-medium p-1.5 w-1/4 border-r border-gray-200">
                 <div className="text-[#231f20]">Select Language</div>
               </td>
               <td className="p-1.5">
                 <div className="flex space-x-2">
                   {languageOptions.map((option) => (
                     <button
                       key={option.value}
                       className={`px-2 py-1 rounded-md flex items-center space-x-1.5 transition-all ${
                         language === option.value
                           // Use the new CSS class
                           ? 'btn-selected-gradient' 
                           : 'bg-gray-100 hover:bg-gray-200 text-gray-700'
                       }`}
                       onClick={() => setLanguage(option.value as Language)}
                     >
                       <div className={`text-base ${language === option.value ? 'text-[#3871a2]' : 'text-[#231f20]'}`}>
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

      {/* Usage Example Code Block */}
      <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
        <div className="flex justify-between items-center bg-gray-50 text-[var(--primary-dark)] px-2 py-1">
          <h3 className="font-medium">
            Example Code ({languageOptions.find(opt => opt.value === language)?.label})
          </h3>
          <button
            onClick={() => copyToClipboard(currentUsageExample)}
            className="flex items-center space-x-1 text-xs bg-white hover:bg-gray-100 rounded px-1.5 py-0.5 transition-colors text-[var(--primary-dark)] border border-gray-200"
          >
            <FontAwesomeIcon icon={copied ? faCheck : faCopy} />
            <span>{copied ? "Copied!" : "Copy"}</span>
          </button>
        </div>
        <SyntaxHighlighter
          language={language === 'bash' ? 'bash' : language}
          style={vscDarkPlus}
          customStyle={{ margin: 0, padding: '0.5rem', borderRadius: '0 0 0.375rem 0.375rem' }}
          wrapLongLines={true}
        >
          {currentUsageExample}
        </SyntaxHighlighter>
      </div>
    </Container>
  );
};

export default UsageExampleGuide;