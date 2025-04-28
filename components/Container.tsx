import React from 'react';

interface ContainerProps {
  children: React.ReactNode;
  className?: string; // Allow optional additional classes
}

const Container: React.FC<ContainerProps> = ({ children, className = '' }) => {
  return (
    <div className={`w-full max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 ${className}`}>
      {children}
    </div>
  );
};

export default Container;