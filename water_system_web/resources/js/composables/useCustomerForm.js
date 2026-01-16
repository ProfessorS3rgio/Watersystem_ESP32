import { ref, reactive } from 'vue'

export function useCustomerForm() {
  const isSubmitting = ref(false)
  const submitError = ref('')
  const submitFieldErrors = ref({})

  const newCustomer = reactive({
    account_no: '',
    customer_name: '',
    address: '',
    previous_reading: 0,
    is_active: true,
  })

  const editCustomer = reactive({
    account_no: '',
    customer_name: '',
    address: '',
    previous_reading: 0,
    is_active: true,
  })

  const resetNewCustomer = () => {
    Object.assign(newCustomer, {
      account_no: '',
      customer_name: '',
      address: '',
      previous_reading: 0,
      is_active: true,
    })
    submitError.value = ''
    submitFieldErrors.value = {}
  }

  const resetEditCustomer = () => {
    Object.assign(editCustomer, {
      account_no: '',
      customer_name: '',
      address: '',
      previous_reading: 0,
      is_active: true,
    })
    submitError.value = ''
    submitFieldErrors.value = {}
  }

  const fieldError = (field) => {
    const v = submitFieldErrors.value?.[field]
    if (!v) return ''
    return Array.isArray(v) ? v[0] : String(v)
  }

  const setFieldErrors = (errors) => {
    submitFieldErrors.value = errors || {}
  }

  const clearErrors = () => {
    submitError.value = ''
    submitFieldErrors.value = {}
  }

  return {
    isSubmitting,
    submitError,
    submitFieldErrors,
    newCustomer,
    editCustomer,
    resetNewCustomer,
    resetEditCustomer,
    fieldError,
    setFieldErrors,
    clearErrors
  }
}