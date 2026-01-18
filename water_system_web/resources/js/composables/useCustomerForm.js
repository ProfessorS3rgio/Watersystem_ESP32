import { ref, reactive } from 'vue'

export function useCustomerForm() {
  const isSubmitting = ref(false)
  const submitError = ref('')
  const submitFieldErrors = ref({})

  const newCustomer = reactive({
    type_id: null,
    customer_name: '',
    brgy_id: null,
    address: '',
    previous_reading: 0,
    status: 'active',
    deduction_id: null,
  })

  const editCustomer = reactive({
    account_no: '',
    type_id: null,
    customer_name: '',
    brgy_id: null,
    address: '',
    previous_reading: 0,
    status: 'active',
    deduction_id: null,
  })

  const resetNewCustomer = () => {
    Object.assign(newCustomer, {
      type_id: null,
      customer_name: '',
      brgy_id: null,
      address: '',
      previous_reading: 0,
      status: 'active',
      deduction_id: null,
    })
    submitError.value = ''
    submitFieldErrors.value = {}
  }

  const resetEditCustomer = () => {
    Object.assign(editCustomer, {
      account_no: '',
      type_id: null,
      customer_name: '',
      brgy_id: null,
      address: '',
      previous_reading: 0,
      status: 'active',
      deduction_id: null,
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