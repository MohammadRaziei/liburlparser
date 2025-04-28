
// Features Section with proper typing
import * as React from "react";
import {FontAwesomeIcon} from "@fortawesome/react-fontawesome";
import {
    faCode,
    faLayerGroup,
    faLightbulb,
    faRocket,
    faShieldAlt,
    faSyncAlt
} from "@fortawesome/free-solid-svg-icons";
import Container from './Container'; // Import the new Container

const FeatureCard = ({ icon, title, description }: {
    icon: React.ReactNode;
    title: string;
    description: string;
}) => (
    <div className="feature-card">
        <div className="flex items-start gap-4 mb-4">
            <div className="feature-icon-container">
                {icon}
            </div>
            <h3 className="text-xl font-semibold text-[var(--primary-dark)] mt-1">
                {title}
            </h3>
        </div>
        <p className="text-gray-600 pl-[60px]">
            {description}
        </p>
    </div>
);

export default function FeaturesSection(){
    const features = [
        {
            icon: <FontAwesomeIcon icon={faCode} className="text-[var(--primary-blue)] text-3xl" />,
            title: "Multiple Language Support",
            description: "Use in Python, C++, and Shell with an intuitive and consistent interface"
        },
        {
            icon: <FontAwesomeIcon icon={faShieldAlt} className="text-[var(--primary-blue)] text-3xl" />,
            title: "Public Suffix List Support",
            description: "Handles known combinatorial suffixes like 'ac.ir' and unknown suffixes"
        },
        {
            icon: <FontAwesomeIcon icon={faLightbulb} className="text-[var(--primary-yellow)] text-3xl" />,
            title: "Clean Code Design",
            description: "Separate Url and Host classes for more organized and maintainable code"
        },
        {
            icon: <FontAwesomeIcon icon={faSyncAlt} className="text-[var(--primary-blue)] text-3xl" />,
            title: "Automatic Updates",
            description: "Public Suffix List updates automatically before each build and deployment"
        },
        {
            icon: <FontAwesomeIcon icon={faRocket} className="text-[var(--primary-yellow)] text-3xl" />,
            title: "High Performance",
            description: "Outperforms other domain extraction libraries in both host and URL parsing"
        },
        {
            icon: <FontAwesomeIcon icon={faLayerGroup} className="text-[var(--primary-blue)] text-3xl" />,
            title: "Cross-Platform",
            description: "Seamless performance across Windows, Linux & macOS"
        }
    ];

    return (
        // Use Container and add vertical padding
        <Container className="py-16"> 
            {/* Remove w-full, max-w-5xl, px-4 from this div */}
            <div> 
                <div className="text-center mb-12">
                    <h2 className="text-3xl font-bold text-[var(--primary-dark)] mb-3">
                        Powerful Features
                    </h2>
                    <p className="text-lg text-gray-600 max-w-2xl mx-auto">
                        Engineered for high-performance URL parsing with minimal dependencies
                    </p>
                </div>

                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-8">
                    {features.map((feature, index) => (
                        <FeatureCard key={index} {...feature} />
                    ))}
                </div>
            </div>
        </Container>
    );
};
