import React, { useState } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faCode, faCopy, faCheck, faLightbulb } from '@fortawesome/free-solid-svg-icons';
import { faPython } from '@fortawesome/free-brands-svg-icons';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/cjs/styles/prism';
import Container from './Container'; // Import the new Container

type Language = 'python' | 'cpp';

const UsageExampleGuide: React.FC = () => {
  const [language, setLanguage] = useState<Language>('python');
  const [copied, setCopied] = useState(false);

  const languageOptions = [
    { value: 'python', label: 'Python', icon: <FontAwesomeIcon icon={faPython} /> },
    { value: 'cpp', label: 'C++', icon: <FontAwesomeIcon icon={faCode} /> },
  ];

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

  const currentUsageExample = usageExamples[language];

  return (
    // Use Container, add vertical padding (e.g., py-12 or py-16)
    <Container className="py-12"> 
        <h2 className="section-title">
          <FontAwesomeIcon icon={faLightbulb} className="text-[var(--primary-yellow)] mr-3" />
          Basic Usage Example
        </h2>

        {/* Content remains the same */}
        {/* Language Selection Row */}
        <div className="bg-white rounded-lg shadow-md overflow-hidden mb-4 border border-gray-200">
         <table className="w-full">
           <tbody>
             <tr className="border-b border-gray-200">
               <td className="bg-gray-50 font-medium p-1.5 w-1/4 border-r border-gray-200">
                 <div className="text-[var(--primary-dark)]">Select Language</div>
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
                       onClick={() => setLanguage(option.value as Language)}
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
           </tbody>
         </table>
      </div>

      {/* Usage Example Code Block */}
      <div className="bg-white rounded-lg shadow-md overflow-hidden border border-gray-200">
        <div className="flex justify-between items-center bg-gray-50 text-[var(--primary-dark)] px-2 py-1">
          <h3 className="font-medium">Example Code ({language === 'python' ? 'Python' : 'C++'})</h3>
          <button
            onClick={() => copyToClipboard(currentUsageExample)}
            className="flex items-center space-x-1 text-xs bg-white hover:bg-gray-100 rounded px-1.5 py-0.5 transition-colors text-[var(--primary-dark)] border border-gray-200"
          >
            <FontAwesomeIcon icon={copied ? faCheck : faCopy} />
            <span>{copied ? "Copied!" : "Copy"}</span>
          </button>
        </div>
        <SyntaxHighlighter
          language={language}
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