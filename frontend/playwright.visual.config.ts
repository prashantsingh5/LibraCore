import {defineConfig} from '@playwright/test';
export default defineConfig({testDir:'./visual-tests',workers:1,use:{baseURL:'http://127.0.0.1:5173'},reporter:'list',webServer:{command:'npm run dev',url:'http://127.0.0.1:5173',reuseExistingServer:true}});
