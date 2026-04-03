import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
  docsSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Getting Started',
      items: [
        'getting-started/hardware-selection',
        'getting-started/installation',
        'getting-started/firmware-flash',
        'getting-started/configuration',
      ],
    },
    {
      type: 'category',
      label: 'Features',
      items: [
        'features/fsd-activation',
        'features/nag-suppression',
        'features/speed-profiles',
        'features/smart-summon',
        'features/emergency-vehicle-detection',
      ],
    },
    {
      type: 'category',
      label: 'Hardware',
      items: [
        'hardware/feather-rp2040',
        'hardware/feather-m4',
        'hardware/esp32',
        'hardware/m5stack',
      ],
    },
    {
      type: 'category',
      label: 'Development',
      items: [
        'development/architecture',
        'development/contributing',
        'development/testing',
      ],
    },
    {
      type: 'category',
      label: 'Wiki',
      items: [
        'wiki/faq',
        'wiki/troubleshooting',
        'wiki/compatibility',
        'wiki/can-bus-basics',
      ],
    },
    {
      type: 'category',
      label: 'Legal',
      items: [
        'legal/disclaimer',
        'legal/impressum',
        'legal/datenschutz',
      ],
    },
  ],
};

export default sidebars;
