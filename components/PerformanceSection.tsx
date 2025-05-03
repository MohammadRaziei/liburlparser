import React from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faTrophy } from '@fortawesome/free-solid-svg-icons';
import Container from './Container';

interface PerformanceItem {
  library: string;
  function: string;
  time: string;
  highlight: boolean;
}

interface PerformanceSectionProps {
  performanceData: PerformanceItem[];
}

const PerformanceSection: React.FC<PerformanceSectionProps> = ({ performanceData }) => {
  return (
    <Container className="py-12">
      <div>
        <h2 className="section-title">
          <FontAwesomeIcon icon={faTrophy} className="text-[#ffd340] mr-3" />
          Performance Comparison
        </h2>
        <p className="text-gray-700 mb-6">
          LibUrlParser outperforms other domain extraction libraries in both host and URL parsing:
        </p>
        <div className="space-y-6">
          <div>
            <h3 className="font-medium text-[#3871a2] mb-4">Extract From Host (10 million domains)</h3>
            <div className="overflow-x-auto rounded-lg shadow border border-gray-200">
              <table className="min-w-full divide-y divide-gray-200">
                <thead className="bg-gray-50">
                  <tr>
                    <th scope="col" className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Library
                    </th>
                    <th scope="col" className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Function
                    </th>
                    <th scope="col" className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">
                      Time
                    </th>
                  </tr>
                </thead>
                <tbody className="bg-white divide-y divide-gray-200">
                  {performanceData.map((item, index) => (
                    <tr key={index} className={item.highlight ? 'bg-gradient-to-r from-[#3871a2]/10 to-[#ffd340]/10' : ''}>
                      <td className={`px-6 py-4 whitespace-nowrap text-sm font-medium ${item.highlight ? 'text-[#231f20]' : 'text-gray-900'}`}>
                        {item.library}
                      </td>
                      <td className="px-6 py-4 whitespace-nowrap text-sm text-gray-500">
                        {item.function}
                      </td>
                      <td className={`px-6 py-4 whitespace-nowrap text-sm ${item.highlight ? 'text-[#231f20] font-semibold' : 'text-gray-500'}`}>
                        {item.time}
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </div>
    </Container>
  );
};

export default PerformanceSection;