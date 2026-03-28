<template>
  <div class="preset-page" v-loading="loading">
    <el-form label-width="120px" label-position="top">

      <!-- 最低版本号 -->
      <el-card shadow="never" class="section-card">
        <template #header>
          <div class="card-header">
            <el-icon><Warning /></el-icon>
            <span>{{ t('preset.versionControl') }}</span>
          </div>
        </template>
        <el-form-item :label="t('preset.minVersion')">
          <el-input
            v-model="form.minVersion"
            :placeholder="t('preset.minVersionPlaceholder')"
            style="width: 100%; max-width: 300px"
            clearable
          />
          <div class="form-tip">{{ t('preset.minVersionTip') }}</div>
        </el-form-item>
      </el-card>

      <!-- WebClient URL -->
      <el-card shadow="never" class="section-card">
        <template #header>
          <div class="card-header">
            <el-icon><Monitor /></el-icon>
            <span>WebClient</span>
          </div>
        </template>
        <el-form-item :label="t('preset.webclientUrl')">
          <el-input
            v-model="form.webclientUrl"
            :placeholder="t('preset.webclientUrlPlaceholder')"
            style="width: 100%; max-width: 500px"
            clearable
          />
          <div class="form-tip">{{ t('preset.webclientUrlTip') }}</div>
        </el-form-item>
      </el-card>

      <!-- 公告 -->
      <el-card shadow="never" class="section-card">
        <template #header>
          <div class="card-header">
            <el-icon><Bell /></el-icon>
            <span>{{ t('preset.announcement') }}</span>
          </div>
        </template>
        <el-tabs>
          <el-tab-pane :label="t('preset.tabZh')">
            <el-form-item :label="t('preset.announcementContent')">
              <el-input
                v-model="form.notice.zh_CN"
                type="textarea"
                :rows="3"
                :placeholder="t('preset.noticePlaceholderZh')"
                style="width: 100%"
              />
            </el-form-item>
          </el-tab-pane>
          <el-tab-pane :label="t('preset.tabEn')">
            <el-form-item :label="t('preset.announcementEn')">
              <el-input
                v-model="form.notice.en_US"
                type="textarea"
                :rows="3"
                :placeholder="t('preset.noticePlaceholderEn')"
                style="width: 100%"
              />
            </el-form-item>
          </el-tab-pane>
        </el-tabs>
        <div class="form-tip">{{ t('preset.announcementTip') }}</div>
      </el-card>

      <!-- 导航链接 -->
      <el-card shadow="never" class="section-card">
        <template #header>
          <div class="card-header">
            <el-icon><Link /></el-icon>
            <span>{{ t('preset.navLinks') }}</span>
          </div>
        </template>
        <el-tabs>
          <el-tab-pane :label="t('preset.tabZh')">
            <LinkEditor v-model="form.links.zh_CN" />
          </el-tab-pane>
          <el-tab-pane :label="t('preset.tabEn')">
            <LinkEditor v-model="form.links.en_US" />
          </el-tab-pane>
        </el-tabs>
        <div class="form-tip">{{ t('preset.navLinksTip') }}</div>
      </el-card>

      <!-- 操作按钮 -->
      <div class="action-bar">
        <el-button type="primary" :loading="saving" @click="handleSave" size="large">
          <el-icon><Check /></el-icon>
          {{ t('preset.saveConfig') }}
        </el-button>
        <el-button @click="handleReset" size="large">
          <el-icon><RefreshLeft /></el-icon>
          {{ t('common.reset') }}
        </el-button>
        <span v-if="lastUpdated" class="last-updated">
          {{ t('common.lastUpdate') }}：{{ lastUpdated }}
        </span>
      </div>
    </el-form>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { ElMessage } from 'element-plus'
import { getPreset, updatePreset } from '../api/preset.js'
import LinkEditor from './LinkEditor.vue'

const { t } = useI18n()

const loading = ref(false)
const saving = ref(false)
const lastUpdated = ref('')

const emptyForm = () => ({
  minVersion: '',
  webclientUrl: '',
  notice: { zh_CN: '', en_US: '' },
  links: { zh_CN: [], en_US: [] }
})

const form = reactive(emptyForm())
let serverSnapshot = null

function parseJsonField(raw, fallback) {
  if (!raw || raw === '') return fallback
  try {
    return JSON.parse(raw)
  } catch {
    return fallback
  }
}

async function loadPreset() {
  loading.value = true
  try {
    const data = await getPreset()
    const notice = parseJsonField(data.notice, { zh_CN: '', en_US: '' })
    const links = parseJsonField(data.links, { zh_CN: [], en_US: [] })

    form.minVersion = data.min_version || ''
    form.webclientUrl = data.webclient_url || ''
    form.notice.zh_CN = notice.zh_CN || ''
    form.notice.en_US = notice.en_US || ''
    form.links.zh_CN = links.zh_CN || []
    form.links.en_US = links.en_US || []

    if (data.updated_at) {
      lastUpdated.value = new Date(data.updated_at).toLocaleString('zh-CN')
    }
    serverSnapshot = JSON.parse(JSON.stringify(form))
  } catch (e) {
    ElMessage.error(t('preset.loadFailed') + ': ' + e.message)
  } finally {
    loading.value = false
  }
}

async function handleSave() {
  saving.value = true
  try {
    const payload = {
      notice: JSON.stringify(form.notice),
      links: JSON.stringify(form.links),
      min_version: form.minVersion,
      webclient_url: form.webclientUrl
    }
    await updatePreset(payload)
    serverSnapshot = JSON.parse(JSON.stringify(form))
    lastUpdated.value = new Date().toLocaleString('zh-CN')
    ElMessage.success(t('preset.saved'))
  } catch (e) {
    ElMessage.error(t('preset.saveFailed') + ': ' + e.message)
  } finally {
    saving.value = false
  }
}

function handleReset() {
  if (serverSnapshot) {
    Object.assign(form, JSON.parse(JSON.stringify(serverSnapshot)))
    ElMessage.info(t('preset.resetDone'))
  }
}

onMounted(loadPreset)
</script>

<style scoped>
.preset-page {
  width: 100%;
  padding: 0 16px;
  box-sizing: border-box;
}

.section-card {
  margin-bottom: 20px;
  width: 100%;
}

.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 600;
  flex-wrap: wrap;
}

.form-tip {
  color: #909399;
  font-size: 12px;
  margin-top: 4px;
}

.action-bar {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 16px 0;
  flex-wrap: wrap;
}

.last-updated {
  color: #909399;
  font-size: 13px;
  margin-left: auto;
}

@media (max-width: 768px) {
  .preset-page {
    padding: 0 12px;
  }

  .action-bar {
    flex-direction: column;
    align-items: flex-start;
    gap: 8px;
  }

  .last-updated {
    margin-left: 0;
    font-size: 12px;
  }
}
</style>
